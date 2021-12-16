#include "CacheServer.h"

// CacheServer构造函数，设置客户端最大连接个数, LRU链表容量
CacheServer::CacheServer(int maxWaiter) : TcpServer(maxWaiter) {
    // 线程池预先开启8个线程
    threadPool = std::make_unique<ThreadPool>(100);
    //
    m_is_migrating = false;  // 默认没有正在进行数据迁移

    m_cache_status = true;  // cache状态默认是良好的，cache发生错误的时候，会把这个状态设置成false
}

void CacheServer::newConnection() {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "deal new connection" << std::endl;
    // 建立新连接在主线程
    while (1) {
        int connfd = accept(m_listen_sockfd, NULL, NULL);  // 获取与客户端相连的fd，但不保存客户端地址信息
        if (connfd < 0) {
            // throw std::runtime_error("accept new connection error\n");
            std::cout << "accept new connection error\n";
            break;
        }

        std::cout << "a new client come, fd = " << connfd << std::endl;

        // 注册与这个客户端有关的事件
        epoll_event ev;
        bzero(&ev, sizeof(ev));
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        // ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
        ev.data.fd = connfd;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
            // throw std::runtime_error("register event error\n");
            std::cout << "register event error\n";
            return;
        }
        setnonblocking(connfd);  // 设置与对端连接的socket文件描述符为非堵塞模式
    }
}

// 现有的连接发生事件，说明对端有传递“信息”过来，虽然有可能是空消息
void CacheServer::existConnection(int event_i) {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "deal existed connection" << std::endl;
    epoll_event ep_ev = this->m_epollEvents[event_i];
    if (ep_ev.events & EPOLLRDHUP) {
        // 有客户端事件发生，但缓冲区没数据可读，说明主动断开连接，于是将其从epoll树上摘除
        int epoll_ctl_ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, ep_ev.data.fd, NULL);
        if (epoll_ctl_ret < 0) {
            throw std::runtime_error("delete client error\n");
        }
        close(ep_ev.data.fd);
        std::cout << "a client left, fd = " << ep_ev.data.fd << std::endl;
    } else if (ep_ev.events & EPOLLIN) {
        // 处理已经存在客户端的请求在子线程处理
        threadPool->enqueue([this, ep_ev]() {  // lambda统一处理即可
            // if (ep_ev.events & EPOLLIN) {
            // 如果与客户端连接的该套接字的输入缓存中有收到数据，则读数据
            char recv_buf_max[MAX_BUF_SIZE];
            bzero(recv_buf_max, sizeof(recv_buf_max));
            int recv_size = recv(ep_ev.data.fd, recv_buf_max, MAX_BUF_SIZE, 0);
            std::cout << "received: " << recv_size << " Bytes" << std::endl;

            if (recv_size <= 0) {
                throw std::runtime_error("recv() error \n");
            } else {
                /* --------------------------- 解析读取到的数据包，并响应数据包的命令 --------------------------- */
                std::cout << "Receive message from client fd: " << ep_ev.data.fd << std::endl;
                CMCData recv_cmc_data;
                recv_cmc_data.ParseFromArray(recv_buf_max, recv_size);
                // 此处先把数据包信息打印出来
                std::string debug_str = recv_cmc_data.DebugString();
                std::cout << debug_str << std::endl;
                std::cout << "DebugString() end!" << std::endl;

                // 判断是否为下线确认数据包，如果是下线包，并且曾经的确申请过下线，则进程结束
                if (recv_cmc_data.data_type() == CMCData::ACKINFO) {
                    if (recv_cmc_data.ack_info().ack_type() == AckInfo::OFFLINEACK) {
                        if (recv_cmc_data.ack_info().ack_status() == AckInfo::OK) {
                            if (offline_applied == true) {
                                std::cout << "cache offline successful!" << std::endl;
                                close(ep_ev.data.fd);  // close fd before exit
                                exit(0);
                            }
                        }
                    }
                }
                // 解析数据包，并生成回复数据包
                CMCData resp_data;                                     // 先定义一个回复数据包，作为parseData传入传出参数
                bool parse_ret = parseData(recv_cmc_data, resp_data);  // 在parseData中会完成数据包的解析，并将响应数据注册进resp_data中
                if (parse_ret == false)
                    std::cout << "parseData() fail, the data format may be wrong!\n";
                else
                    std::cout << "parseData() successful!\n";

                // 接下来要把响应数据包序列化后再传回客户端
                std::string resp_data_str = resp_data.DebugString();
                std::cout << "resp_data: \n"
                        << resp_data_str << std::endl;

                int resp_data_size = resp_data.ByteSizeLong();
                std::cout << "data_size: " << resp_data_size << std::endl;
                char send_buf_max[MAX_BUF_SIZE];
                bzero(send_buf_max, sizeof(send_buf_max));  // 少了这句
                resp_data.SerializeToArray(send_buf_max, resp_data_size);
                std::cout << "after SerializeToArray: " << strlen(send_buf_max) << std::endl;

                // 回复客户端
                int ret = send(ep_ev.data.fd, send_buf_max, resp_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << ret << std::endl;
                if (ret < 0) {
                    std::cout << "error on send()\n";
                    return;
                }
            }  
        });
    } else {  // 未知错误
        std::cout << "unknown error\n";
        return;
    }
}

// 解析数据包，传入recv_data进行分析，然后修改response_data回复数据包
bool CacheServer::parseData(const CMCData& recv_data, CMCData& response_data) {
    // 判断数据类型，分类处理
    switch (recv_data.data_type()) {
        // 如果是命令数据包，则执行命令并传出回复数据包
        case CMCData::COMMANDINFO: {
            return executeCommand(recv_data.cmd_info(), response_data);
        }

        // 如果是哈希槽信息包，则根据新的哈希槽进行数据迁移
        //（有可能是整个槽的信息，也有可能是某个节点的增减信息）
        // 1、一般刚上线的cache第一次会收到master发来的ALLCACHEINFO
        // 这时你需要：m_hashslot_new.restoreFrom(const HashSlotInfo&)
        // 2、第一次以后收到哈希槽信息包，master一般是发来单个节点的变动信息（ADDCACHE/REMCACHE）
        // 这时你需要：m_hashslot_new.addCacheNode(const CacheNode&)或m_hashslot_new.remCacheNode(const CacheNode&)
        // 并在完成数据迁移后，向response_data中写入HASHSLOTUPDATEACK数据包信息（表达完成数迁完成的确认）
        case CMCData::HASHSLOTINFO: {
            std::cout << "CMCData::HASHSLOTINFO:" << std::endl;
            if (dataMigration(recv_data.hs_info(), response_data)) {
                // 更新本地old哈希槽
                std::lock_guard<std::mutex> lock_g(this->m_hashslot_mutex);  // 为更新本地old哈希槽操作上锁
                this->m_hashslot = this->m_hashslot_new;                     // 更新本地old哈希槽
                return true;
            }
        }
        default:
            break;
    }
    return false;
}

// 心跳线程函数
bool CacheServer::beginHeartbeatThread(const struct sockaddr_in& master_addr, sockaddr_in& cache_addr) {
    auto enqueue_ret = threadPool->enqueue([this, &master_addr, &cache_addr]() {
        /* ---------------------与master建立持久心跳连接--------------------- */
        // 创建与master相连的心跳专用套接字文件描述符
        int heart_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (heart_sock < 0) {
            perror("socket() error\n");
            return false;
        }
        int opt = 1;
        setsockopt(heart_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        // 阻塞连接master
        if (connect(heart_sock, (struct sockaddr*)&master_addr, sizeof(sockaddr_in)) < 0) {
            perror("connect() error\n");
            close(heart_sock);
            return false;
        }
        socklen_t len = sizeof(cache_addr);
        getsockname(heart_sock, (sockaddr*)&cache_addr, &len);
        // cout << "heartbeart thread connect master server success!" << endl;

        char heart_send_buff[BUFSIZ];
        int data_size;
        HeartInfo heart_info;
        // 定时发送心跳给master
        bool print_success_msg = true;  // 第一次心跳发送成功时打印成功的消息，后续不打印
        while (1) {
            // 检查offline_apply_flag这个下线标志全局变量是否为true
            //如果管理员正在申请下线，则此次发送下线数据包，不发送心跳数据包
            if (offline_applying) {
                std::cout << "offline_apply_flag: true" << std::endl;
                /*-------------制作下线数据包并序列化-------------*/
                CommandInfo offline_cmd_info;
                offline_cmd_info.set_cmd_type(CommandInfo::OFFLINE);

                CMCData offline_data;
                offline_data.set_data_type(CMCData::COMMANDINFO);
                auto cmd_info_ptr = offline_data.mutable_cmd_info();
                cmd_info_ptr->CopyFrom(offline_cmd_info);

                data_size = offline_data.ByteSizeLong();
                offline_data.SerializeToArray(heart_send_buff, data_size);
                offline_applying = false;  // 下线申请标志复位
            } else {
                /*-------------制作心跳包并序列化-------------*/
                heart_info.set_cur_time(time(NULL));
                heart_info.set_cache_status(this->m_cache_status ? HeartInfo::OK : HeartInfo::FAIL);
                CMCData heart_cmc_data;
                heart_cmc_data.set_data_type(CMCData::HEARTINFO);
                auto ht_info_ptr = heart_cmc_data.mutable_ht_info();
                ht_info_ptr->CopyFrom(heart_info);

                data_size = heart_cmc_data.ByteSizeLong();
                heart_cmc_data.SerializeToArray(heart_send_buff, data_size);
            }

            /*-------------向master端发送心跳/下线数据-------------*/
            if (this->m_epoll_is_ready) {
                int send_size = send(heart_sock, heart_send_buff, data_size, 0);
                if (send_size > 0 && print_success_msg) {
                    std::cout << "Heartbeat thread started successfully!" << send_size << std::endl;
                    print_success_msg = false;
                } else if (send_size < 0) {
                    std::cout << "heartbeat sending failed!" << std::endl;
                    return false;
                }
                sleep(1);  // 一秒发送一次心跳
            }
        }
    });

    return true;
}

// 执行相应的命令
// 传入命令信息，传出回复数据包
bool CacheServer::executeCommand(const CommandInfo& cmd_info, CMCData& response_data) {
    switch (cmd_info.cmd_type()) {
        /*----------------------------- GET -----------------------------*/
        case CommandInfo::GET: {
            std::string key = cmd_info.param1();
            std::cout << "key: " << key << std::endl;
            std::cout << "param2: " << cmd_info.param2().empty() << std::endl;
            // 如果命令数据包中的：key字段不为空，且param2()(即value)为空，则查询并注册响应数据包response_data
            if (!key.empty() && cmd_info.param2().empty()) {
                std::string value;
                {
                    // 先尝试查询，查询成功则value不为空，后续返回结果包即可，失败则检查是否正在数据迁移
                    std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.get操作上锁
                    std::cout << "will value = m_cache.get(key);" << std::endl;
                    value = m_cache.get(key);
                    std::cout << "value" << value << std::endl;
                }
                // 如果在本cache的查询不为空，则直接返回此结果即可
                if (!value.empty()) {
                    std::cout << "value: " << value << std::endl;
                    // 封装KvData键值包
                    KvData kv_data;
                    kv_data.set_key(key);
                    kv_data.set_value(value);
                    // 注册封装CMCData数据包
                    response_data.set_data_type(CMCData::KVDATA);
                    auto kv_data_ptr = response_data.mutable_kv_data();
                    kv_data_ptr->CopyFrom(kv_data);
                    return true;  // 函数正常返回
                } else {          // 如果本cache表查询为空，则检查是否正在数据迁移
                    // 如果正在进行数据迁移，此时客户端查询的那个value有可能已经被转移到新节点了，此时需要帮客户端去查询另外一个节点
                    if (this->m_is_migrating) {
                        // 根据客户端提供的key去查询新的哈希槽，向新的cache发送GET命令包，收到回复后再转发给客户端
                        char another_cache_ip[16];                                                       // 另一台cache主机ip
                        strcpy(another_cache_ip, this->m_hashslot_new.getCacheAddr(key).first.c_str());  // 从hashslot中查询cache地址
                        int another_cache_port = this->m_hashslot_new.getCacheAddr(key).second;
                        // 这是帮忙查询的数据包（发给另外一个cache的）
                        CMCData another_cmc_data;
                        another_cmc_data = MakeCommandData(CommandInfo::GET, key, "");
                        // 发送查询命令数据包到另外一台主机，SendCommandData函数内部会写入收到的结果信息于response_data中并传出
                        if (SendCommandData(another_cmc_data, another_cache_ip, another_cache_port, response_data) == false)
                            std::cout << "send GET command to another cache fail." << std::endl;
                        return true;  // 函数正常返回
                    } else {
                        // 查询为空，且cache不在数据数据迁移，返回空结果给客户端即可
                        std::cout << "value: " << value << std::endl;
                        // 封装KvData键值包
                        KvData kv_data;
                        kv_data.set_key(key);
                        kv_data.set_value(value);
                        // 注册封装CMCData数据包
                        response_data.set_data_type(CMCData::KVDATA);
                        auto kv_data_ptr = response_data.mutable_kv_data();
                        kv_data_ptr->CopyFrom(kv_data);
                        return true;  // 函数正常返回
                    }
                }
            }
            return false;  // 查询格式不合法的情况，没能正确传出response_data回复数据包
        }

        /*----------------------------- SET -----------------------------*/
        case CommandInfo::SET: {
            std::string key = cmd_info.param1();
            std::string value = cmd_info.param2();
            // 如果待写入的key和value字段都不为空，则为合法操作
            if (!key.empty() && !value.empty()) {
                bool set_ret;  // set成功与否的标志

                // 查询该key原有的所属地址（也就是本机cache地址）
                char cache_ip_self[16];  // 根据hashslot(old)查询的ip
                strcpy(cache_ip_self, inet_ntoa(this->m_serv_addr.sin_addr));
                int cache_port_self = ntohs(this->m_serv_addr.sin_port);
                // 查询该key最新的所属cache地址
                char cache_ip_new[16];
                strcpy(cache_ip_new, m_hashslot_new.getCacheAddr(key).first.c_str());
                int cache_port_new = m_hashslot_new.getCacheAddr(key).second;

                // 先判断是否正在进行数据迁移
                if (!this->m_is_migrating) {
                    // 虽然此时本机cache没有正在进行数据迁移
                    // 但此时假如其他节点还在数据迁移，此时master还没通知client更新slot
                    // 假如client拿着旧hashslot找到本cache并试图set到本地，这是不允许的
                    // 此时需要加个判断这个set操作是否为本地cache负责
                    // 假如是本地负责，则set到本地cache
                    // 不是本地负责的set操作，则转发到新cache中（注意转发SET成功后不用删除本地，因为本地已经没有这个key了）
                    if (!strcmp(cache_ip_new, cache_ip_self) && cache_port_new == cache_port_self) {
                        // 如果新旧地址是同一个，说明确实为本地cache负责的set
                        // set到本地cache
                        std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.get操作上锁
                        set_ret = m_cache.set(key, value);                        // set kv to LRU_cache
                    } else {
                        // 否则为该key已不是由本地cache所负责，转发SET
                        // 这是发给新cache的SET数据包value
                        CMCData another_cmc_data;
                        another_cmc_data = MakeCommandData(CommandInfo::SET, key, value);
                        // 发送SET命令数据包到另外一台主机，SendCommandData函数内部会写入收到的结果信息于response_data中并传出
                        CMCData set_another_ret_data;
                        if (SendCommandData(another_cmc_data, cache_ip_new, cache_port_new, set_another_ret_data) == false)
                            std::cout << "send SET command to another cache fail." << std::endl;
                        // 看看set_another_ret_data这个回复包里有没有SETACK，有的话就返回SET成功
                        if (set_another_ret_data.data_type() == CMCData::ACKINFO) {
                            if (set_another_ret_data.ack_info().ack_type() == AckInfo::SETACK) {
                                if (set_another_ret_data.ack_info().ack_status() == AckInfo::OK)
                                    set_ret = true;
                                else
                                    set_ret = false;
                            }
                        }
                    }
                } else {
                    /* ----- 否则说明正在进行数据迁移，此时需要分类讨论 ----- */
                    // 1、根据hashslot_new查询待set的k-v键值对是否仍为本机所管理，如果是，则直接set到本地
                    // 2、如果发现这个k-v是由另外一台caceh所存储，则需要给另外一台cache发送SET命令，当收到SETACK后删除本地k-v
                    if (!strcmp(cache_ip_new, cache_ip_self) && cache_port_new == cache_port_self) {
                        std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.get操作上锁
                        set_ret = m_cache.set(key, value);                        // set kv to LRU_cache
                    } else {
                        // 这是发给新cache的SET数据包value
                        CMCData another_cmc_data;
                        another_cmc_data = MakeCommandData(CommandInfo::SET, key, value);
                        // 发送SET命令数据包到另外一台主机，SendCommandData函数内部会写入收到的结果信息于response_data中并传出
                        CMCData set_another_ret_data;
                        if (SendCommandData(another_cmc_data, cache_ip_new, cache_port_new, set_another_ret_data) == false)
                            std::cout << "send SET command to another cache fail." << std::endl;
                        // 看看set_another_ret_data这个回复包里有没有SETACK，有的话就将本地的这对k-v删除
                        if (set_another_ret_data.data_type() == CMCData::ACKINFO) {
                            if (set_another_ret_data.ack_info().ack_type() == AckInfo::SETACK) {
                                if (set_another_ret_data.ack_info().ack_status() == AckInfo::OK) {
                                    {
                                        std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.deleteKey操作上锁
                                        set_ret = m_cache.deleteKey(key);                         // 删除k-v键值对
                                    }
                                }
                            }
                        }
                    }
                }
                // 定义一个确认包
                AckInfo ack_info;
                ack_info.set_ack_type(AckInfo::SETACK);  // 设置确认包类型

                /* 根据查询状态不同来进一步封装确认包，并注册CMCData数据包 */
                AckInfo_AckStatus ack_status = (set_ret == true) ? AckInfo::OK : AckInfo::FAIL;
                ack_info.set_ack_status(ack_status);
                // 将确认包注册封装成CMCData数据包
                response_data.set_data_type(CMCData::ACKINFO);
                auto ack_info_ptr = response_data.mutable_ack_info();
                ack_info_ptr->CopyFrom(ack_info);
                return true;
            }
            return false;  // 查询格式不合法的情况，没能正确传出response_data回复数据包
        }

        /*----------------------------- DEL -----------------------------*/
        case CommandInfo::DEL: {
            std::string key = cmd_info.param1();
            if (!key.empty() && cmd_info.param2().empty()) {
                bool del_ret;  // del成功与否的标志
                {
                    // 先尝试DEL本地key，能直接删除成功就直接删除，返回DELACK数据包就行了
                    std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.deleteKey操作上锁
                    del_ret = m_cache.deleteKey(key);                         // 删除k-v键值对
                }
                // 如果删除成功，则返回DELACK成功数据包
                if (del_ret == true) {
                    // 定义一个确认包
                    AckInfo ack_info;
                    ack_info.set_ack_type(AckInfo::DELACK);  // 设置确认包类型
                    ack_info.set_ack_status(AckInfo::OK);
                    // 将确认包注册封装成CMCData数据包
                    response_data.set_data_type(CMCData::ACKINFO);
                    auto ack_info_ptr = response_data.mutable_ack_info();
                    ack_info_ptr->CopyFrom(ack_info);
                    return true;
                } else {
                    // 如果删除失败，则考虑是否正在进行数据迁移，可能导致想删除的那个k-v已经迁移到另外一台cache了
                    if (this->m_is_migrating) {
                        // 根据客户端提供的key去查询新的哈希槽，向新的cache发送DEL命令包，收到回复后再转发给客户端
                        char another_cache_ip[16];                                                       // 另一台cache主机ip
                        strcpy(another_cache_ip, this->m_hashslot_new.getCacheAddr(key).first.c_str());  // 从hashslot中查询cache地址
                        int another_cache_port = this->m_hashslot_new.getCacheAddr(key).second;
                        // 这是请求帮忙的DEL数据包（发给另外一个cache的）
                        CMCData another_cmc_data;
                        another_cmc_data = MakeCommandData(CommandInfo::DEL, key, "");
                        // 发送删除命令数据包到另外一台主机，SendCommandData函数内部会写入收到的结果信息于response_data中并传出
                        if (SendCommandData(another_cmc_data, another_cache_ip, another_cache_port, response_data) == false)
                            std::cout << "send DEL commandData to another cache fail." << std::endl;
                        return true;  // 函数正常返回
                    } else {
                        // 如果本地删除失败且没有正在进行数据迁移，则返回删除失败ACK数据包
                        // 定义一个确认包
                        AckInfo ack_info;
                        ack_info.set_ack_type(AckInfo::DELACK);  // 设置确认包类型
                        ack_info.set_ack_status(AckInfo::FAIL);
                        // 将确认包注册封装成CMCData数据包
                        response_data.set_data_type(CMCData::ACKINFO);
                        auto ack_info_ptr = response_data.mutable_ack_info();
                        ack_info_ptr->CopyFrom(ack_info);
                        return true;
                    }
                }
            }
            return false;  // 查询格式不合法的情况，没能正确传出response_data回复数据包
        }

        default:
            return false;
    }
}

// 数据迁移处理函数
// 注意：在这个函数里不用新开线程进行数据迁移了
// 因为这个dataMigration()就是在existConnection()函数的内部新开的线程被嵌套调用的
// 这个数据迁移函数本身就是在一个独立的线程里进行数据迁移了
// 当cache收到其他主机发来的数据后，仍然是可以在主线程里被epoll监听到的，并在其他线程得到响应
bool CacheServer::dataMigration(const HashSlotInfo& hs_info, CMCData& response_data) {
    // 在这个函数里，你需要做这些事情：
    // 因为这里hs_info有可能是整个槽的信息，也有可能是某个节点的增减信息
    // 所以这里需要加个判断
    // 1、一般刚上线的cache第一次会收到master发来的ALLCACHEINFO
    // 这时你需要调用：m_hashslot_new.restoreFrom(const HashSlotInfo&)
    // 2、第一次以后收到哈希槽信息包，master一般是发来单个节点的变动信息（ADDCACHE/REMCACHE）
    // 这时你需要调用：m_hashslot_new.addCacheNode(const CacheNode&)或m_hashslot_new.remCacheNode(const CacheNode&)
    // 并在完成数据迁移后，向response_data中写入HASHSLOTUPDATEACK数据包信息（表达完成数迁完成的确认）

    // 根据收到哈希槽的类型来更新本地最新的那一份哈希槽m_hashslot_new
    switch (hs_info.hashinfo_type()) {
        // 1、假如收到包含所有节点的哈希槽信息
        case HashSlotInfo::ALLCACHEINFO: {
            // 更新本地hashslot_new，注意这里不要把old的hashslot给覆盖了
            std::lock_guard<std::mutex> lock_g(this->m_hashslot_new_mutex);  // 为更新本地最新的哈希槽操作上锁
            m_hashslot_new.restoreFrom(hs_info);
            break;
        }

        // 2、假如收到单个节点的add哈希槽变动数据包
        case HashSlotInfo::ADDCACHE: {
            // 更新本地hashslot_new，注意这里不要把old的hashslot给覆盖了
            std::string add_cache_ip = hs_info.cache_node().ip();
            int add_cache_port = hs_info.cache_node().port();
            {
                std::lock_guard<std::mutex> lock_g(this->m_hashslot_new_mutex);  // 为更新本地最新的哈希槽操作上锁
                m_hashslot_new.addCacheNode(CacheNode(add_cache_ip, add_cache_port));
            }
            break;
        }

        // 2、假如收到单个节点的rem哈希槽变动数据包
        case HashSlotInfo::REMCACHE: {
            // 更新本地hashslot_new，注意这里不要把old的hashslot给覆盖了
            std::string rem_cache_ip = hs_info.cache_node().ip();
            int rem_cache_port = hs_info.cache_node().port();
            {
                std::lock_guard<std::mutex> lock_g(this->m_hashslot_new_mutex);  // 为更新本地最新的哈希槽操作上锁
                m_hashslot_new.remCacheNode(CacheNode(rem_cache_ip, rem_cache_port));
            }
            break;
        }

        default:
            break;
    }

    // 主要思路就是：

    // 先检查新槽是否是空槽

    // 1、遍历LRU链表的k-v键值对，根据key在hashslot_new查一下该键值对最新的分布情况（cache地址）
    // 类似于这样查询：
    // char cache_ip_new[16];         // 新cache主机ip
    // strcpy(cache_ip_new, m_hashslot_new.getCacheAddr(本cacheLRU中的某个key).first.c_str());
    // int cache_port_new = m_hashslot_new.getCacheAddr(本cacheLRU中的某个key).second;
    // 查到后就对比cache_ip_new/cache_port_new是否仍为本机的地址，假如是本机，就跳过这个k-v，考察下一个
    // 假如发现当前的k-v不是本机负责，则模仿client发送SET命令（你可以看一下client的发送命令的代码）给另外一台主机
    // 并把本机的该k-v从LRU链表中删除

    /*--------------- 数据迁移开始 ---------- -----*/
    this->m_is_migrating = true;
    // 遍历本cacheLRU中的所有key，根据hashslot_new查询本地key是否还属于本机管理，如果是，则跳过，否则set到新cache地址。
    if (!m_cache.m_map.empty()) {
        // 查询本地ip&port
        char cache_ip_self[16];
        // strcpy(cache_ip_self, m_hashslot.getCacheAddr(m_cache.m_map.begin()->first).first.c_str());
        // int cache_port_self = m_hashslot.getCacheAddr(m_cache.m_map.begin()->first).second;
        strcpy(cache_ip_self, inet_ntoa(this->m_serv_addr.sin_addr));
        int cache_port_self = ntohs(this->m_serv_addr.sin_port);
        char cache_ip_new[16];
        int cache_port_new;

        if (m_hashslot_new.numNodes() != 0) {   // 是不是应该加个🔓？
            auto kv_self = m_cache.m_map.begin();
            while (kv_self != m_cache.m_map.end()) {
                std::string key_self = kv_self->first;
                strcpy(cache_ip_new, m_hashslot_new.getCacheAddr(key_self).first.c_str());
                cache_port_new = m_hashslot_new.getCacheAddr(key_self).second;
                if (strcmp(cache_ip_new, cache_ip_self) != 0 || cache_port_new != cache_port_self) {
                    // 新hashsort中key不在本机，连接cache_ip_new对应的cache，再向新cache地址发送SET命令
                    CMCData migrating_data;
                    std::string str_key_self;
                    {
                        std::lock_guard<std::mutex> lock(m_cache_mutex);
                        str_key_self = m_cache.get(key_self);
                    }
                    migrating_data = MakeCommandData(CommandInfo::SET, key_self, str_key_self);
                    std::cout << "will SendCommandData for migrating" << std::endl;
                    CMCData result_data;                                                                      // 访问的结果数据包
                    if (SendCommandData(migrating_data, cache_ip_new, cache_port_new, result_data) == false)  // 发送命令数据包给cache
                        std::cout << "sendCommandData for migrating fail." << std::endl;
                    // 检查另外一台主机回复的result_data中是否为SETACK & OK，没问题就删除本地的这个kv

                    bool del_ret;  // del成功与否的标志
                    if (result_data.data_type() == CMCData::ACKINFO) {
                        if (result_data.ack_info().ack_type() == AckInfo::SETACK) {
                            if (result_data.ack_info().ack_status() == AckInfo::OK) {
                                std::lock_guard<std::mutex> lock_g(this->m_cache_mutex);  // 为m_cache.deleteKey操作上锁
                                ++kv_self;                                                // 防止k-vd键值对的删除造成迭代器的紊乱
                                del_ret = m_cache.deleteKey(key_self);                    // 删除k-v键值对
                            }
                        }
                    }
                    if (del_ret == false) {
                        std::cout << "delete local k-v fail." << std::endl;
                        return false;
                    }
                } else {
                    ++kv_self;
                }
            }
            // 打印本地k-v键值队情况
            std::cout << "local k-v:" << std::endl;
            for (auto kv_now : m_cache.m_map) {
                std::cout << kv_now.first << " = " << kv_now.second << std::endl;
                std::cout << "--------------------------------" << std::endl;
            }
            std::cout << "Local cache data migration completed." << std::endl;
        } else {
            // shutting down this last cache server will lost all cached data
            std::cout << "Shutting down the last cache server!" << std::endl;
        }
    }
    // ----------------> code in here <-------------------- //

    this->m_is_migrating = false;
    /*--------------- 数据迁移完成 ---------- -----*/

    /*--------------- 赋值写入回复数据包：response_data（传入传出参数） ---------- -----*/
    // 把数据迁移完成的HASHSLOTUPDATEACK信息放进这里面就行了：CMCData& response_data
    AckInfo update_ack;
    update_ack.set_ack_type(AckInfo::HASHSLOTUPDATEACK);
    update_ack.set_ack_status(AckInfo::OK);

    response_data.set_data_type(CMCData::ACKINFO);
    auto ack_info_ptr = response_data.mutable_ack_info();
    ack_info_ptr->CopyFrom(update_ack);
    // 写入response_data完成
    return true;  // dataMigration return
}
