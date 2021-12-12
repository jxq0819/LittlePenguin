#include "MasterServer.h"

#include "cmcdata.pb.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <chrono>

// MasterServer构造函数，设置客户端最大连接个数
MasterServer::MasterServer(int maxWaiter) : TcpServer(maxWaiter) {
    // 线程池预先开启8个线程
    threadPool = std::make_unique<ThreadPool>(8);
    hash_slot_ = std::make_unique<HashSlot>();    // New empty hash_slot
}

void MasterServer::init() {
    std::thread heartbeat_service(startHeartbeartService, this);
    heartbeat_service.detach();
    std::thread hashslot_service(startHashslotService, this);
    hashslot_service.detach();
}

void MasterServer::newConnection() {
    // std::cout << "-------------------------------------------" << std::endl;
    // std::cout << "New connection" << std::endl;
    // 建立新连接在主线程
    sockaddr_in new_addr;
    socklen_t len = sizeof(new_addr);
    int connfd = accept(m_listen_sockfd, (sockaddr*) &new_addr, &len);  // 获取与客户端相连的fd，但不保存客户端地址信息
    if (connfd < 0) {
        std::cout << "accept new connection error\n";
    }
    // record the Addr {ip,port} 
    char p_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(new_addr.sin_addr.s_addr), p_addr, sizeof(p_addr));    // convert IP address from numeric to presentation
    links_.insert({std::string(p_addr), ntohs(new_addr.sin_port)});                    // add the Addr of the new connection to [links_]
    std::cout << p_addr << ":" << ntohs(new_addr.sin_port) << " has connected" << std::endl;

    // 注册与这个客户端有关的事件
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // 注册：读事件、ET边沿模式、对端断开连接或半关闭事件
    ev.data.fd = connfd;
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
        std::cout << "register event error\n";
        return;
    }
    setnonblocking(connfd);
}

void MasterServer::existConnection(int event_i) {
    // std::cout << "-------------------------------------------" << std::endl;
    // std::cout << "existing connection\n";
    // 处理已经存在客户端的请求在子线程处理
    threadPool->enqueue([this, event_i]() {  // lambda统一处理即可
        if (this->m_epollEvents[event_i].events & EPOLLRDHUP) {
            // 有客户端事件发生，但缓冲区没数据可读，说明主动断开连接
            // remove the Addr {ip:port} from links_
            sockaddr_in left_addr;
            socklen_t len = sizeof(left_addr);
            bzero(&left_addr, len);
            getpeername(this->m_epollEvents[event_i].data.fd, (sockaddr*) &left_addr, &len);
            // record the Addr {ip:port} 
            char p_addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(left_addr.sin_addr.s_addr), p_addr, sizeof(p_addr)); // convert the IP address from numeric to presentation
// acquire lock
{           std::lock_guard<std::mutex> lock(links_mutex_);
            links_.erase({std::string(p_addr), ntohs(left_addr.sin_port)});
}
// acquire lock
{           std::lock_guard<std::mutex> lock(client_links_mutex_);
            client_links_.erase({std::string(p_addr), ntohs(left_addr.sin_port)});                // remove if exist, otherwise do nothing
}
            std::cout << p_addr << ":" << ntohs(left_addr.sin_port) << " left" << std::endl;
            // 将其从epoll树上摘除
            if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_epollEvents[event_i].data.fd, NULL) < 0) {
                throw std::runtime_error("delete client error\n");
            }
        } else if (this->m_epollEvents[event_i].events & EPOLLIN) {
            // 如果与客户端连接的该套接字的输入缓存中有收到数据
            char buf[MAX_BUFFER];
            memset(buf, 0, MAX_BUFFER);
            int recv_size = recv(m_epollEvents[event_i].data.fd, buf, MAX_BUFFER, 0);
            // std::cout << "received: " << recv_size << " Byte" << std::endl;
            if (recv_size <= 0) {
                throw std::runtime_error("recv() error \n");
            } else {
                // std::cout << "Receive message from client fd: " << m_epollEvents[event_i].data.fd << std::endl;
                CMCData recv_cmc_data;
                recv_cmc_data.ParseFromArray(buf, recv_size);
                // bzero(buf, sizeof(buf));
                // std::string debug_str = recv_cmc_data.DebugString();
                // std::cout << debug_str << std::endl;
                // record the Addr {ip:port} 
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                getpeername(m_epollEvents[event_i].data.fd, (sockaddr*) &addr, &len);
                char p_addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr.sin_addr.s_addr), p_addr, sizeof(p_addr));        // convert the IP address from numeric to presentation
                // process the received cmc_data
                CMCData send_cmc_data;
                HashSlotInfo send_hs_info;
                HeartbeatInfo recv_ht_info;
                int send_size;
                switch (recv_cmc_data.data_type()) {
                    case CMCData::COMMANDINFO:
                        std::cout << "Command received" << std::endl;
                        switch (recv_cmc_data.cmd_info().cmd_type()) {
                            case CommandInfo::GETSLOT:
                                std::cout << "Command GETSLOT received" << std::endl;
// acquire lock
{                               std::lock_guard<std::mutex> lock(client_links_mutex_);
                                client_links_.insert({std::string(p_addr),ntohs(addr.sin_port)});  // add the current Addr to [client_link_]
}
                                // 返回Client完整的HashSlotInfo
                                char send_buff[102400];
                                bzero(send_buff, 102400);
                                send_cmc_data.set_data_type(CMCData::HASHSLOTINFO);
// acquire lock
{                               std::lock_guard<std::mutex> lock(hash_slot_mutex_);
                                hash_slot_->saveTo(send_hs_info);
}
                                send_cmc_data.mutable_hs_info()->CopyFrom(send_hs_info);
                                std::cout << send_cmc_data.DebugString() << std::endl;                   // debug string
                                std::cout << "Data Size: " << send_cmc_data.ByteSizeLong() << std::endl;
                                send_cmc_data.SerializeToArray(send_buff, BUFSIZ);
                                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                                send_size = send(m_epollEvents[event_i].data.fd, send_buff, send_cmc_data.ByteSizeLong(), 0);
                                std::cout << "send_size: " << send_size << std::endl;
                                if (send_size < 0) {
                                    std::cout << "Send hashslot change failed!" << std::endl;
                                }
                                // wait for HASHSLOTUPDATEACK
                                if (checkAckInfo(m_epollEvents[event_i].data.fd, AckInfo::HASHSLOTUPDATEACK)) {
                                    std::cout << "HASHSLOTUPDATEACK from client received!" << std::endl;
                                } else {
                                    std::cout << "HASHSLOTUPDATEACK from client receive error!" << std::endl;
                                }
                                break;
                            // Other cases
                            case CommandInfo::OFFLINE:
                                std::cout << "OFFLINE request received" << std::endl;
// acquire lock
{                               std::lock_guard<std::mutex> lock(cache_links_mutex_);
                                if (cache_links_.find({std::string(p_addr), ntohs(addr.sin_port)}) == cache_links_.end()) {
                                    std::cout << "OFFLINE request from a unrecorded cache server!" << std::endl;
                                } else {
                                    time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                    cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}].time = t;          // 收到OFFLINE包时也更新对应节点的心跳时间戳
                                    task_queue_.Push({TASK_SHUT, {std::string(p_addr), ntohs(addr.sin_port)}});  // 缩容，add a task to deal with the leaving cache server
                                }
}
                                break;
                            default:;
                        }
                    break;
                    case CMCData::HEARTINFO:
                        std::cout << "Heartbeat from " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                        recv_ht_info.time = recv_cmc_data.ht_info().cur_time();
                        recv_ht_info.status = recv_cmc_data.ht_info().cache_status();
// acquire lock
{                       std::lock_guard<std::mutex> lock(cache_links_mutex_);
                        if (cache_links_.count({std::string(p_addr), ntohs(addr.sin_port)})) {
                            cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}] = recv_ht_info;        // update Heartbeat Info
                        } else {
                            cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}] = recv_ht_info;        // Heartbeat from a new cache server
                            task_queue_.Push({TASK_ADD, {std::string(p_addr), ntohs(addr.sin_port)}});       // 扩容，add a task to deal with the new cache server
                        }
}
                        break;
                    // Other cases
                    default:;
                }
            }
        } else {  // 未知错误
            throw std::runtime_error("unknown error");
            // std::cout << "unknown error\n";
            return;
        }
    });
}

int MasterServer::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void MasterServer::startHeartbeartService(MasterServer *m)
{
    std::cout << "Started heartbeat thread!\n";
    while (1) {
        std::cout << "-------------------------------------------" << std::endl;
        time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());  // get the current time in seconds
        // check if all cache servers are ok
        bool all_good = true;
// acquire lock
{       std::lock_guard<std::mutex> lock(m->cache_links_mutex_);
        for (auto it = m->cache_links_.begin(); it != m->cache_links_.end(); ++it) {
            std::cout << "Current time is " << t << ", cache " << it->first.first << ":" << it->first.second << " last seen at " << it->second.time << std::endl;
            if (t - it->second.time > 1) {    // Heartbeat lost
                std::cout << "Lost cache server " << it->first.first << ":" << it->first.second << "\n";
                m->task_queue_.Push({TASK_LOST, it->first});    // add a task to deal with the lost cache server
                m->cache_links_.erase(it);                      // remove the lost cache server from cache_links_
                all_good = false;
                // std::cout << *(m->hash_slot_) << std::endl;
            }
        }
}
        // 维护黑名单
// acquire lock
{       std::lock_guard<std::mutex> lock(m->blacklist_mutex_);
        for (auto it = m->blacklist_.begin(); it != m->blacklist_.end(); ++it) {
            if (t - it->second > TIMEOUT) {
                std::cout << it->first.first << ":" << it->first.second << " removed from the blacklist\n" << std::endl;
                m->blacklist_.erase(it);
            }
        }
}
        if (all_good) { 
            // std::cout << "All cache servers are good\n";
            std::cout << *(m->hash_slot_) << std::endl;
        }
        std::cout << "-------------------------------------------" << std::endl;
        sleep(1);
    }
}

void MasterServer::startHashslotService(MasterServer *m)
{
    std::cout << "Started hashslot thread!\n";
    while (1) {
        auto task = m->task_queue_.Pop();   // get a pending task, will BLOCK the thread if the queue is empty
        if (task.first == TASK_ADD) {
            // 1. add the new node to the cache list
            // 2. notify all (excluding the new node) of the CHANGE of the hashslot
            // 3. wait for each node to respond HASHSLOTUPDATEACK
            // 4. notify the new node of the COMPLETE new hashslot
            // 5. wait for the new node to respond HASHSLOTUPDATEACK
            // 6. replace the old hashslot
            // 7. notify all clients of the CHANGE of the hashslot
            std::unique_ptr<HashSlot> new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_)); // copy the current HashSlot
            Addr addr = {task.second.first, task.second.second};                                    // Addr of the new cache server
            new_hash_slot->addCacheNode(CacheNode(addr.first, addr.second));                        // modify the new hashslot
            // sequentially connect each old cache server
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cur < new_hash_slot->numNodes() - 1; ++cur, ++cache_it) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the cache server address
                const char* cache_ip = cache_it->ip().c_str();         // cache server's ip
                const u_int16_t cache_port = cache_it->port();         // cache server's port
                std::cout << cache_port << std::endl;
                if (cache_port <= 1024 || cache_port >= 65535) {
                    perror("cache_port error");
                }
                struct sockaddr_in cache_addr;
                bzero(&cache_addr, sizeof(cache_addr));
                cache_addr.sin_family = AF_INET;
                cache_addr.sin_port = htons(cache_port);
                if (inet_pton(AF_INET, cache_ip, &cache_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create a socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 暂用阻塞连接每个旧节点，默认75s等待；当多个cache server同时变化的情况下会出问题。需改用select合理设置超时时间（略大于心跳间隔+RTT） 
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // if connect returns error, it means that the current cache server has already left
                    continue;   // continue to deal with the next cache server
                }
                // construct and send hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::ADDCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // wait for HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
                }
                close(sockfd);
            }
            // then send to the new cache server the complete hashslot info, BUFSIZ 8192 too small ?
            char send_buff[102400];
            bzero(send_buff, 102400);
            // record the cache server address
            const auto &cache_node = new_hash_slot->cback();
            const char* cache_ip = cache_node.ip().c_str();         // cache server's ip
            const u_int16_t cache_port = cache_node.port();         // cache server's port
            if (cache_port <= 1024 || cache_port >= 65535) {
                perror("cache_port error");
            }
            struct sockaddr_in cache_addr;
            bzero(&cache_addr, sizeof(cache_addr));
            cache_addr.sin_family = AF_INET;
            cache_addr.sin_port = htons(cache_port);
            if (inet_pton(AF_INET, cache_ip, &cache_addr.sin_addr.s_addr) < 0) {
                perror("inet_pton() error\n");
            }
            // create a socket file descriptor
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket() error\n");
            }
            // 暂用阻塞连接新节点，默认75s等待；当多个cache server同时变化的情况下会出问题。需合理设置超时时间（略大于心跳间隔+RTT） 
            if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                perror("connect() error\n");
                close(sockfd);
            }
            // construct and send the complete hashslot
            CMCData cmc_data;
            cmc_data.set_data_type(CMCData::HASHSLOTINFO);
            HashSlotInfo hs_info;
            new_hash_slot->saveTo(hs_info);
            cmc_data.mutable_hs_info()->CopyFrom(hs_info);
            std::cout << cmc_data.DebugString() << std::endl;                   // debug string
            std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
            cmc_data.SerializeToArray(send_buff, BUFSIZ);
            std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
            int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
            std::cout << "send_size: " << send_size << std::endl;
            if (send_size < 0) {
                std::cout << "Send the complete hashslot failed!" << std::endl;
            }
            // wait for HASHSLOTUPDATEACK
            if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
            } else {
                std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
            }
            close(sockfd);
// acquire lock
{           std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
            // then transfer the ownership of the hashslot ptr and notify each client of the new hashslot
            m->hash_slot_.reset(new_hash_slot.release());         
}     
            std::unordered_set<Addr, addrHasher> client_links;
// acquire lock
{           std::lock_guard<std::mutex> lock(m->client_links_mutex_);
            // copy intialise a local replica of the client list
            client_links = m->client_links_;
}
            // sequentially connect each client and send the hashslot change info
            for (auto &client : client_links) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the client address
                const char* client_ip = client.first.c_str();        // client ip
                const u_int16_t client_port = client.second;         // client port
                if (client_port <= 1024 || client_port >= 65535) {
                    perror("client_port error");
                }
                struct sockaddr_in client_addr;
                bzero(&client_addr, sizeof(client_addr));
                client_addr.sin_family = AF_INET;
                client_addr.sin_port = htons(client_port);
                if (inet_pton(AF_INET, client_ip, &client_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 阻塞连接Client
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // if connect returns error, it means that the current client has already left
                    continue;   // continue to deal with the next client
                }
                // construct and send hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::ADDCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // Debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // Block accept HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from client received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from client receive error!" << std::endl;
                }
                close(sockfd);
            }
            std::cout << "TASK_ADD Completed!" << std::endl;
        } else if (task.first == TASK_LOST) {
            // 1. remove the lost node from the cache list
            // 2. notify all (excluding the lost node) of the CHANGE of the hashslot
            // 3. wait for each node to respond HASHSLOTUPDATEACK
            // 4. replace the old hashslot
            // 5. notify all clients of the CHANGE of the hashslot
            std::unique_ptr<HashSlot> new_hash_slot;
// acquire lock
{           std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
            new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_));              // copy the current HashSlot
}
            Addr addr = {task.second.first, task.second.second};                        // Addr of the cache server to remove
            new_hash_slot->remCacheNode(CacheNode(addr.first, addr.second));            // modify the new hashslot
            // sequentially connect each remaining cache server
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cache_it != new_hash_slot->cend(); ++cache_it) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the cache server adress
                const char* cache_ip = cache_it->ip().c_str();         // cache server ip
                const u_int16_t cache_port = cache_it->port();         // cache server port
                if (cache_port <= 1024 || cache_port >= 65535) {
                    perror("cache_port error");
                }
                struct sockaddr_in cache_addr;
                bzero(&cache_addr, sizeof(cache_addr));
                cache_addr.sin_family = AF_INET;
                cache_addr.sin_port = htons(cache_port);
                if (inet_pton(AF_INET, cache_ip, &cache_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create a socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 阻塞连接
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // if connect returns error, it means that the current cache server has already left
                    continue;   // continue to deal with the next cache server
                }
                // construct and send hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // Debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // wait for HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
                }
                close(sockfd);
            }
            // then transfer the ownership of the hashslot ptr and notify each client of the new hashslot
// acquire lock
{           std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
            m->hash_slot_.reset(new_hash_slot.release());
}
            // copy intialise a local replica of the list of clients
            auto client_links = m->client_links_;   // needs lock
            // connect each client and send hashslot change info
            for (auto &client : client_links) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the client addess
                const char* client_ip = client.first.c_str();        // client ip
                const u_int16_t client_port = client.second;         // client port
                if (client_port <= 1024 || client_port >= 65535) {
                    perror("client_port error");
                }
                struct sockaddr_in client_addr;
                bzero(&client_addr, sizeof(client_addr));
                client_addr.sin_family = AF_INET;
                client_addr.sin_port = htons(client_port);
                if (inet_pton(AF_INET, client_ip, &client_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create a socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 阻塞连接client
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // if connect returns error, it means that the current client has already left
                    continue;   // continue to deal with the next client
                }
                // construct and send the hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // wait for HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from client received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from client receive error!" << std::endl;
                }
                close(sockfd);
            }
            std::cout << "TASK_LOST Completed!" << std::endl;
        
        } else if (task.first == TASK_SHUT) {
            // 1. remove the leaving node from the cache list
            // 2. notify all (including the leaving node) of the CHANGE of the hashslot
            // 3. wait for each node to respond HASHSLOTUPDATEACK
            // 4. after notifying the leaving node OFFLINEACK, add it to the blacklist, ignore furthur heartbeat from the same node
            // 5. then remove the already left node from cache_links_
            // 6. replace the old hashslot
            // 7. notify all clients of the CHANGE fo the hashslot
            std::unique_ptr<HashSlot> new_hash_slot;
// acquire lock
{           std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
            new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_));               // copy the current HashSlot
}
            Addr addr = {task.second.first, task.second.second};                        // Addr of the leaving cache server
            new_hash_slot->remCacheNode(CacheNode(addr.first, addr.second));            // modify the new hashslot
            // sequentially connect each remaining cache server
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cache_it != new_hash_slot->cend(); ++cache_it) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the cache server adress
                const char* cache_ip = cache_it->ip().c_str();         // cache server ip
                const u_int16_t cache_port = cache_it->port();         // cache server port
                if (cache_port <= 1024 || cache_port >= 65535) {
                    perror("cache_port error");
                }
                struct sockaddr_in cache_addr;
                bzero(&cache_addr, sizeof(cache_addr));
                cache_addr.sin_family = AF_INET;
                cache_addr.sin_port = htons(cache_port);
                if (inet_pton(AF_INET, cache_ip, &cache_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create a socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 阻塞连接
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // If connect returns error, it means that the current cache server has already left
                    continue;   // continue to deal with the next cache server
                }
                // construct and send hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // Debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // wait for HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
                }
                close(sockfd);
            }
            // then send to the leaving cache server the hashslot change info
            char send_buff[BUFSIZ];
            bzero(send_buff, BUFSIZ);
            // record the cache server address
            const char* cache_ip = addr.first.c_str();         // the leaving cache server's ip
            const u_int16_t cache_port = addr.second;          // the leaving cache server's port
            if (cache_port <= 1024 || cache_port >= 65535) {
                perror("cache_port error");
            }
            struct sockaddr_in cache_addr;
            bzero(&cache_addr, sizeof(cache_addr));
            cache_addr.sin_family = AF_INET;
            cache_addr.sin_port = htons(cache_port);
            if (inet_pton(AF_INET, cache_ip, &cache_addr.sin_addr.s_addr) < 0) {
                perror("inet_pton() error\n");
            }
            // create a socket file descriptor
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket() error\n");
            }
            // 暂用阻塞连接离开节点
            if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                perror("connect() error\n");
                close(sockfd);
            }
            // construct and send the hashslot change info
            CMCData cmc_data;
            cmc_data.set_data_type(CMCData::HASHSLOTINFO);
            HashSlotInfo hs_info;
            new_hash_slot->saveTo(hs_info);
            cmc_data.mutable_hs_info()->CopyFrom(hs_info);
            std::cout << cmc_data.DebugString() << std::endl;                   // debug string
            std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
            cmc_data.SerializeToArray(send_buff, BUFSIZ);
            std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
            int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
            std::cout << "send_size: " << send_size << std::endl;
            if (send_size < 0) {
                std::cout << "Send hashslot change failed!" << std::endl;
            }
            // wait for HASHSLOTUPDATEACK
            if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
            } else {
                std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
            }
            // send OFFLINEACK
            bzero(send_buff, BUFSIZ);
            AckInfo ack_info;
            ack_info.set_ack_type(AckInfo::OFFLINEACK);
            ack_info.set_ack_status(AckInfo::OK);
            cmc_data.Clear();
            cmc_data.set_data_type(CMCData::ACKINFO);
            cmc_data.mutable_ack_info()->CopyFrom(ack_info);
            cmc_data.SerializeToArray(send_buff, BUFSIZ);
            send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
            if (send_size < 0) {
                std::cout << "Send OFFLINEACK failed!" << std::endl;
            }
            // add the permitted-to-leave node to the blacklist
            time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
// acquire lock
{           std::lock_guard<std::mutex> lock(m->blacklist_mutex_);
            m->blacklist_[{addr.first, addr.second}] = t;
}
// acquire lock
{           std::lock_guard<std::mutex> lock(m->cache_links_mutex_);
            // remove the permitted-to-leave node from the cache_links_
            m->cache_links_.erase({addr.first, addr.second});
}
            close(sockfd);
// acquire lock
{           std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
            // then transfer the ownership of the hashslot ptr and notify each client of the new hashslot
            m->hash_slot_.reset(new_hash_slot.release());
}
            std::unordered_set<Addr, addrHasher> client_links;
// acquire lock
{           std::lock_guard<std::mutex> lock(m->client_links_mutex_);
            // copy intialise a local replica of the list of clients
            client_links = m->client_links_;
}
            // connect each client and send hashslot change info
            for (auto &client : client_links) {
                char send_buff[BUFSIZ];
                bzero(send_buff, BUFSIZ);
                // record the client addess
                const char* client_ip = client.first.c_str();        // client ip
                const u_int16_t client_port = client.second;         // client port
                if (client_port <= 1024 || client_port >= 65535) {
                    perror("client_port error");
                }
                struct sockaddr_in client_addr;
                bzero(&client_addr, sizeof(client_addr));
                client_addr.sin_family = AF_INET;
                client_addr.sin_port = htons(client_port);
                if (inet_pton(AF_INET, client_ip, &client_addr.sin_addr.s_addr) < 0) {
                    perror("inet_pton() error\n");
                }
                // create a socket file descriptor
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // 阻塞连接client
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // If connect returns error, it means that the current client has already left
                    continue;   // continue to deal with the next client
                }
                // construct and send the hashslot change info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFSIZ);
                std::cout << "After SerializeToArray: " << strlen(send_buff) << std::endl;
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
                std::cout << "send_size: " << send_size << std::endl;
                if (send_size < 0) {
                    std::cout << "Send hashslot change failed!" << std::endl;
                }
                // wait for HASHSLOTUPDATEACK
                if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                    std::cout << "HASHSLOTUPDATEACK from client received!" << std::endl;
                } else {
                    std::cout << "HASHSLOTUPDATEACK from client receive error!" << std::endl;
                }
                close(sockfd);
            }
            std::cout << "TASK_SHUT Completed!" << std::endl;

        } else {
            std::cout << "Undefined Hashslot operation!" << std::endl;
        }
        // sleep(1);
    }
}

// Utility functions
bool checkAckInfo(int sockfd, int ackType)
{
    char recv_buff[BUFSIZ];
    bzero(recv_buff, BUFSIZ);
    int recv_size = recv(sockfd, recv_buff, BUFSIZ, 0);
    if (recv_size <= 0) {
        std::cout << "ACK receive error!" << std::endl;
    } else {
        std::cout << "received: " << recv_size << " Byte" << std::endl;
        CMCData recv_cmc_data;
        recv_cmc_data.ParseFromArray(recv_buff, recv_size);
        if (recv_cmc_data.data_type() == CMCData::ACKINFO) {
            if (recv_cmc_data.ack_info().ack_type() == ackType) {
                return true;
            }
        }
    }
    return false;
}
