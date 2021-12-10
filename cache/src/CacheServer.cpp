#include "CacheServer.h"

// CacheServer构造函数，设置客户端最大连接个数, LRU链表容量
CacheServer::CacheServer(int maxWaiter) : TcpServer(maxWaiter) {
    // 线程池预先开启8个线程
    threadPool = std::make_unique<ThreadPool>(10);
    //
    m_is_migrating = false;  // 默认没有正在进行数据迁移

    m_cache_status = true;  // cache状态默认是良好的，cache发生错误的时候，会把这个状态设置成false
}

void CacheServer::newConnection() {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "deal new connection" << std::endl;
    // 建立新连接在主线程
    int connfd = accept(m_listen_sockfd, NULL, NULL);  // 获取与客户端相连的fd，但不保存客户端地址信息
    if (connfd < 0) {
        // throw std::runtime_error("accept new connection error\n");
        std::cout << "accept new connection error\n";
    }

    std::cout << "a new client come\n";

    // 注册与这个客户端有关的事件
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    // ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = connfd;
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
        // throw std::runtime_error("register event error\n");
        std::cout << "register event error\n";
        return;
    }
    setnonblocking(connfd);  // 设置与对端连接的socket文件描述符为非堵塞模式
}

// 现有的连接发生事件，说明对端有传递“信息”过来，虽然有可能是空消息
void CacheServer::existConnection(int event_i) {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "deal existed connection" << std::endl;
    // 处理已经存在客户端的请求在子线程处理
    threadPool->enqueue([this, event_i]() {  // lambda统一处理即可
        epoll_event ep_ev = this->m_epollEvents[event_i];
        if (ep_ev.events & EPOLLRDHUP) {
            // 有客户端事件发生，但缓冲区没数据可读，说明主动断开连接，于是将其从epoll树上摘除
            std::lock_guard<std::mutex> lock_g(this->m_mutex);  // 为epoll_ctl操作上锁
            int epoll_ctl_ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, ep_ev.data.fd, NULL);
            if (epoll_ctl_ret < 0) {
                throw std::runtime_error("delete client error\n");
            }
            std::cout << "a client left" << std::endl;
        } else if (ep_ev.events & EPOLLIN) {
            // 如果与客户端连接的该套接字的输入缓存中有收到数据，则读数据
            char recv_buf_max[MAX_BUF_SIZE];
            memset(recv_buf_max, 0, sizeof(recv_buf_max));
            int recv_size = recv(ep_ev.data.fd, recv_buf_max, MAX_BUF_SIZE, 0);
            std::cout << "received: " << recv_size << " Bytes" << std::endl;

            if (recv_size <= 0) {
                throw std::runtime_error("recv() error \n");
            } else {
                /* --------------------------- 解析读取到的数据包，并响应数据包的命令 --------------------------- */
                std::cout << "Receive message from client fd: " << ep_ev.data.fd << std::endl;
                CMCData recv_cmc_data;
                recv_cmc_data.ParseFromArray(recv_buf_max, sizeof(recv_buf_max));

                // 此处先把数据包信息打印出来
                string debug_str = recv_cmc_data.DebugString();
                cout << debug_str << endl;
                cout << "DebugString() end!" << endl;

                // 解析数据包，并生成回复数据包
                CMCData resp_data;                                     // 先定义一个回复数据包，作为parseData传入传出参数
                bool parse_ret = parseData(recv_cmc_data, resp_data);  // 在parseData中会完成数据包的解析，并将响应数据注册进resp_data中
                if (parse_ret == false)
                    std::cout << "parseData() fail!\n";
                else
                    std::cout << "parseData() successful!\n";

                // 接下来要把响应数据包序列化后再传回客户端
                string resp_data_str = resp_data.DebugString();
                cout << "resp_data: \n"
                     << resp_data_str << endl;

                int resp_data_size = resp_data.ByteSizeLong();
                cout << "data_size: " << resp_data_size << endl;
                char send_buf_max[MAX_BUF_SIZE];
                resp_data.SerializeToArray(send_buf_max, resp_data_size);
                cout << "after SerializeToArray: " << strlen(send_buf_max) << endl;

                // 回复客户端
                int ret = send(ep_ev.data.fd, send_buf_max, strlen(send_buf_max), 0);
                cout << "send_size: " << ret << endl;
                if (ret < 0) {
                    std::cout << "error on send()\n";
                    return;
                }
            }
        } else {  // 未知错误
            std::cout << "unknown error\n";
            return;
        }
    });
}

// 解析数据包
bool CacheServer::parseData(const CMCData& recv_data, CMCData& response_data) {
    // 判断数据类型，分类处理
    switch (recv_data.data_type()) {
        // 如果是命令数据包
        case CMCData::COMMANDINFO: {
            return executeCommand(recv_data.cmd_info(), response_data);
        } break;

        // 如果是键值对数据包（在数据迁移时会用到）
        case CMCData::KVDATA: {
            ;
        } break;

        // 如果是哈希槽信息包，则根据新的哈希槽进行数据迁移
        //（有可能是整个槽的信息，也有可能是某个节点的增减信息）
        case CMCData::HASHSLOTINFO: {
            return dataMigration();
        } break;

        default:
            break;
    }
    return false;
}

// 心跳线程函数
bool CacheServer::beginHeartbeatThread(const struct sockaddr_in& master_addr) {
    auto enqueue_ret = threadPool->enqueue([this, &master_addr]() {
        /* ---------------------与master建立持久心跳连接--------------------- */
        // 创建与master相连的心跳专用套接字文件描述符
        int heart_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (heart_sock < 0) {
            perror("socket() error\n");
            return false;
        }
        // 阻塞连接master
        if (connect(heart_sock, (struct sockaddr*)&master_addr, sizeof(sockaddr_in)) < 0) {
            perror("connect() error\n");
            close(heart_sock);
            return false;
        }

        // cout << "heartbeart thread connect master server success!" << endl;

        char heart_send_buff[BUFSIZ];
        HeartInfo heart_info;
        // 定时发送心跳给master
        bool print_success_msg = true;  // 第一次心跳发送成功时打印成功的消息，后续不打印
        while (1) {
            /*-------------制作心跳包并序列化-------------*/
            heart_info.set_cur_time(time(NULL));
            heart_info.set_cache_status(m_cache_status ? HeartInfo::OK : HeartInfo::FAIL);
            CMCData heart_cmc_data;
            heart_cmc_data.set_data_type(CMCData::HEARTINFO);
            auto ht_info_ptr = heart_cmc_data.mutable_ht_info();
            ht_info_ptr->CopyFrom(heart_info);

            int data_size = heart_cmc_data.ByteSizeLong();
            heart_cmc_data.SerializeToArray(heart_send_buff, data_size);

            /*-------------向master端发送心跳数据-------------*/
            int send_size = send(heart_sock, heart_send_buff, data_size, 0);
            // cout << "heart_send_size: " << send_size << endl;
            if (send_size > 0 && print_success_msg) {
                cout << "Heartbeat thread started successfully!" << send_size << endl;
                print_success_msg = false;
            } else if (send_size < 0) {
                std::cout << "heartbeat sending failed!" << std::endl;
                return false;
            }
            sleep(1);  // 一秒发送一次心跳
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
            string key = cmd_info.param1();
            // 如果命令数据包中的：key字段不为空，且param2()(即value)为空，则查询并注册响应数据包response_data
            if (!key.empty() && cmd_info.param2().empty()) {
                string value;
                {
                    std::lock_guard<std::mutex> lock_g(this->m_mutex);  // 为m_cache.get操作上锁
                    value = m_cache.get(key);                           // 查询结果
                }
                if (value.empty()) return false;  // 查询为空则说明查询失败
                // 封装KvData键值包
                KvData kv_data;
                kv_data.set_key(key);
                kv_data.set_value(value);
                // 注册封装CMCData数据包
                response_data.set_data_type(CMCData::KVDATA);
                auto kv_data_ptr = response_data.mutable_kv_data();
                kv_data_ptr->CopyFrom(kv_data);
                return true;
            } else
                return false;
        }
        /*----------------------------- SET -----------------------------*/
        case CommandInfo::SET: {
            string key = cmd_info.param1();
            string value = cmd_info.param2();
            // 如果待写入的key和value字段都不为空，则为合法操作
            if (!key.empty() && !value.empty()) {
                bool set_ret;
                {
                    std::lock_guard<std::mutex> lock_g(this->m_mutex);  // 为m_cache.get操作上锁
                    set_ret = m_cache.set(key, value);                  // set kv to LRU_cache
                }
                // 定义一个确认包
                AckInfo ack_info;
                ack_info.set_ack_type(AckInfo::SETACK);  // 设置确认包类型

                /* 根据查询状态不同来进一步封装确认包，并注册CMCData数据包 */
                AckInfo_AckStatus ack_status;
                if (set_ret == true)
                    ack_status = AckInfo::OK;
                else
                    ack_status = AckInfo::FAIL;

                ack_info.set_ack_status(ack_status);
                // 将确认包注册封装成CMCData数据包
                response_data.set_data_type(CMCData::ACKINFO);
                auto ack_info_ptr = response_data.mutable_ack_info();
                ack_info_ptr->CopyFrom(ack_info);

                return true;
            }
        }
        /*----------------------------- DEL -----------------------------*/
        case CommandInfo::DEL: {
            string key = cmd_info.param1();
            // 如果命令数据包中的：key字段不为空，且param2()(即value)为空，则删除指定key-value，并返回AckInfo
            if (!key.empty() && cmd_info.param2().empty()) {
                bool del_ret;
                {
                    std::lock_guard<std::mutex> lock_g(this->m_mutex);  // 为m_cache.deleteKey操作上锁
                    del_ret = m_cache.deleteKey(key);                   // 删除k-v键值对
                }

                // 定义一个确认包
                AckInfo ack_info;
                ack_info.set_ack_type(AckInfo::DELACK);  // 设置确认包类型
                /* 根据删除结果状态不同来进一步封装确认包，并注册CMCData数据包 */
                AckInfo_AckStatus ack_status;
                if (del_ret == true)
                    ack_status = AckInfo::OK;
                else
                    ack_status = AckInfo::FAIL;
                ack_info.set_ack_status(ack_status);
                // 将确认包注册封装成CMCData数据包
                response_data.set_data_type(CMCData::ACKINFO);
                auto ack_info_ptr = response_data.mutable_ack_info();
                ack_info_ptr->CopyFrom(ack_info);
                return true;
            }
        }

        default:
            return false;
    }
}

// 数据迁移处理函数

bool CacheServer::dataMigration(const HashSlotInfo &hs_info, CMCData &response_data) {
    ;
}