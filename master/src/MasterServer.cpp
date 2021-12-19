#include "MasterServer.h"

#include "cmcdata.pb.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>

#include <iostream>
#include <string>
#include <thread>
#include <utility>

MasterServer::MasterServer(int maxWaiter) : TcpServer(maxWaiter) {
    // on start: create 10 threads in the thead pool
    threadPool = std::make_unique<ThreadPool>(10);
    hash_slot_ = std::make_unique<HashSlot>();
}

void MasterServer::init() {
    std::thread heartbeat_service(startHeartbeartService, this);
    heartbeat_service.detach();
    std::thread hashslot_service(startHashslotService, this);
    hashslot_service.detach();
}

void MasterServer::newConnection() {
    // accept new connections in the main thread
    sockaddr_in new_addr;
    socklen_t len = sizeof(new_addr);
    int connfd = accept(m_listen_sockfd, (sockaddr*) &new_addr, &len);
    if (connfd < 0) {
        std::cerr << "accept new connection error\n";
        return;
    }
    // record the address and the port number of the new connection in a list of std::pair(s) of {ip, port} 
    char p_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(new_addr.sin_addr.s_addr), p_addr, sizeof(p_addr));
    { 
        std::lock_guard<std::mutex> lock(links_mutex_);
        links_.insert({std::string(p_addr), ntohs(new_addr.sin_port)});  // insert the new peer's address and port number to the list of all established links
    }
    std::cout << p_addr << ":" << ntohs(new_addr.sin_port) << " has connected" << std::endl;
    // register epoll evenets for the new connection
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;  // epoll events: readable, edge-triggered, hang up
    ev.data.fd = connfd;
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
        std::cerr << "register event error\n";
        return;
    }
    setnonblocking(connfd);
}

void MasterServer::existConnection(int event_i) {
    epoll_event ep_ev = this->m_epollEvents[event_i];  // initialise a copy of the current epoll event
    if (ep_ev.events & EPOLLRDHUP) {
        // the link has been closed, remove the peer from the list of all established links
        sockaddr_in left_addr;
        socklen_t len = sizeof(left_addr);
        bzero(&left_addr, len);
        getpeername(ep_ev.data.fd, (sockaddr*) &left_addr, &len);
        char p_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(left_addr.sin_addr.s_addr), p_addr, sizeof(p_addr));
        {
            std::lock_guard<std::mutex> lock(links_mutex_);
            links_.erase({std::string(p_addr), ntohs(left_addr.sin_port)});
        }
        {
            std::lock_guard<std::mutex> lock(client_links_mutex_);
            client_links_.erase({std::string(p_addr), ntohs(left_addr.sin_port)});
        }
        std::cout << p_addr << ":" << ntohs(left_addr.sin_port) << " left" << std::endl;
        // remove the fd from the epoll tree
        if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, ep_ev.data.fd, NULL) < 0) {
            throw std::runtime_error("delete client error\n");
        }
        close(ep_ev.data.fd);  // close and release the fd
    } else if (ep_ev.events & EPOLLIN) {
        // deal with all the commands or requests in the thread pool using lambdas
        threadPool->enqueue([this, ep_ev]() {
            char recv_buf[BUFF_SIZE];
            char send_buff[BUFF_SIZE_LONG];
            bzero(recv_buf, sizeof(recv_buf));
            int recv_size = recv(ep_ev.data.fd, recv_buf, BUFF_SIZE, 0);
            if (recv_size <= 0) {
                std::cerr << "recv() error \n";
                return;
            } else {
                // local objects and variables
                CMCData recv_cmc_data;
                CMCData send_cmc_data;
                CommandInfo recv_cmd_info;
                HashSlotInfo send_hs_info;
                HeartbeatInfo recv_ht_info;
                int send_size;
                bool inBlacklist = false;  // if the heartbeat from {ip, port} is ignored
                // record the peer's socket information
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                getpeername(ep_ev.data.fd, (sockaddr*) &addr, &len);
                char p_addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr.sin_addr.s_addr), p_addr, sizeof(p_addr));
                // then parse and process the received packet
                recv_cmc_data.ParseFromArray(recv_buf, recv_size);
                switch (recv_cmc_data.data_type()) {
                    case CMCData::COMMANDINFO:
                        recv_cmd_info = recv_cmc_data.cmd_info();
                        switch (recv_cmd_info.cmd_type()) {
                            case CommandInfo::GETSLOT:
                                std::cout << "Command GETSLOT from client " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                                {
                                    std::lock_guard<std::mutex> lock(client_links_mutex_);
                                    client_links_.insert({std::string(p_addr),ntohs(addr.sin_port)});  // GETSLOT indicates that the current peer is a client
                                }
                                {
                                    std::lock_guard<std::mutex> lock(hash_slot_mutex_);
                                    hash_slot_->saveTo(send_hs_info);  // save the current hashslot info
                                }
                                // send back to the client the complete hashslot information
                                bzero(send_buff, BUFF_SIZE_LONG);
                                send_cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                                send_cmc_data.mutable_hs_info()->CopyFrom(send_hs_info);
                                send_cmc_data.SerializeToArray(send_buff, BUFF_SIZE_LONG);
                                send_size = send(ep_ev.data.fd, send_buff, send_cmc_data.ByteSizeLong(), 0);
                                std::cout << "Complete HashSlot (" << send_size << " bytes) returned to client  " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                                if (send_size < 0) {
                                    std::cerr << "Send hashslot change failed!\n";
                                }
                                // wait for HASHSLOTUPDATEACK -- WILL BE CAPTURED BY EPOLL since it is a active connection from the client, thus registered in epoll
                                break;
                            // Other cases
                            case CommandInfo::OFFLINE:
                                std::cout << "OFFLINE request from cache server " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                                {
                                    std::lock_guard<std::mutex> lock(cache_links_mutex_);
                                    if (cache_links_.find({std::string(p_addr), ntohs(addr.sin_port)}) == cache_links_.end()) {
                                        std::cout << "OFFLINE request from an unknown cache server!" << std::endl;
                                    } else {
                                        cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}].time = std::time(0);  // update the heartbeat timestamp of the leaving cache server
                                        cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}].status = 0;
                                        task_queue_.Push({TASK_SHUT, {std::string(p_addr), ntohs(addr.sin_port)}});  // 主动缩容
                                    }
                                }
                                break;
                            default:
                                std::cerr << "Unexpected command from " << p_addr << ":" << ntohs(addr.sin_port) << "!\n";
                        }
                        break;
                    case CMCData::HEARTINFO:
                        std::cout << "Heartbeat from " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                        recv_ht_info.time = recv_cmc_data.ht_info().cur_time();
                        recv_ht_info.status = recv_cmc_data.ht_info().cache_status();
                        if (std::time(0) - recv_ht_info.time > DELAY_TIMEOUT) {
                            std::cout << "Heartbeat rejected. Please check your system time or network status." << std::endl;  // 丢弃到达时刻与发送时刻超过一定时间间隔的心跳包
                        } else {
                            {
                                std::lock_guard<std::mutex> lock(blacklist_mutex_);
                                inBlacklist = blacklist_.count({std::string(p_addr), ntohs(addr.sin_port)}); // check if the heartbeat is from a cache server in the blacklist
                            }
                            if (!inBlacklist) {
                                std::lock_guard<std::mutex> lock(cache_links_mutex_);
                                if (cache_links_.count({std::string(p_addr), ntohs(addr.sin_port)})) {
                                    cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}] = recv_ht_info;  // update the existing eartbeat Info
                                } else {
                                    cache_links_[{std::string(p_addr), ntohs(addr.sin_port)}] = recv_ht_info;  // heartbeat from a new cache server
                                    task_queue_.Push({TASK_ADD, {std::string(p_addr), ntohs(addr.sin_port)}});  // 扩容
                                }
                            }
                        }
                        break;
                    case CMCData::ACKINFO:
                        std::cout << "HASHSLOTUPDATEACK from " << p_addr << ":" << ntohs(addr.sin_port) << std::endl;
                        break;
                    default:
                        std::cerr << "Undefined message received!\n";
                }
            }
        });
    } else {
        throw std::runtime_error("unknown error");
        return;
    }
}

int MasterServer::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;

}

// check the heartbeats and update the blacklist at every second
void MasterServer::startHeartbeartService(MasterServer *m)
{
    std::cout << "Heartbeat service running!\n";
    while (1) {
        std::cout << "-------------------------------------------" << std::endl;
        time_t t = std::time(0);  // get the current time in seconds
        bool all_good = true;
        {  // check the heartbeats
            std::lock_guard<std::mutex> lock(m->cache_links_mutex_);
            for (auto it = m->cache_links_.begin(); it != m->cache_links_.end(); /* empty */) {
                std::cout << "Current time is " << t << ", cache server " << it->first.first << ":" << it->first.second << " last seen at " << it->second.time << std::endl;
                if (t - it->second.time > HEARTBEAT_TIMEOUT) {  // if a cache server dies
                    std::cout << "Lost cache server " << it->first.first << ":" << it->first.second << std::endl;
                    m->task_queue_.Push({TASK_LOST, it->first});  // 被动缩容，会出现数据丢失
                    it = m->cache_links_.erase(it);  // remove the it from the list of all living cache servers
                    all_good = false;
                } else {
                    ++it;
                }
            }
        }
        {  // update the blacklist     
            std::lock_guard<std::mutex> lock(m->blacklist_mutex_);
            for (auto it = m->blacklist_.begin(); it != m->blacklist_.end(); /* empty */) {
                if (t - it->second > BLACKLIST_TIMEOUT) {
                    std::cout << it->first.first << ":" << it->first.second << " removed from the blacklist" << std::endl;
                    it = m->blacklist_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        if (all_good) { 
            std::cout << *(m->hash_slot_) << std::endl;
            std::cout << "Clients:\n";
            { std::lock_guard<std::mutex> lock(m->client_links_mutex_);
                for (auto &client : m->client_links_) {
                    std::cout << client.first << ":" << client.second << std::endl;
                }
            }
        }
        std::cout << "-------------------------------------------" << std::endl;
        sleep(1);
    }
}

// retrive and perform tasks (TASK_ADD/TASK_SHUT/TASK_LOST) from the task queue
void MasterServer::startHashslotService(MasterServer *m)
{
    std::cout << "Hashslot service running!\n";
    while (1) {
        auto task = m->task_queue_.Pop();  // get a pending task, will BLOCK the thread if the queue is empty
        if (task.first == TASK_ADD) {
            // 1. add the new node (cache server) to the node list
            // 2. notify the new node of the COMPLETE new hashslot and wait for HASHSLOTUPDATEACK
            // 3. sequentially notify each old node of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            // 4. replace the old hashslot
            // 5. notify all clients of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            std::unique_ptr<HashSlot> new_hash_slot;
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_));  // copy the current HashSlot
            }
            // ---------------- 1 ----------------
            Addr addr = {task.second.first, task.second.second};  // {ip, port} of the new node
            new_hash_slot->addCacheNode(CacheNode(addr.first, addr.second));
            // ---------------- 2 ----------------
            char send_buff[BUFF_SIZE_LONG];
            bzero(send_buff, BUFF_SIZE_LONG);
            const auto &cache_node = new_hash_slot->cback();
            const char* cache_ip = cache_node.ip().c_str();  // cache server's ip
            const u_int16_t cache_port = cache_node.port();  // cache server's port
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
            // create a socket
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket() error\n");
            }
            // block connect the new node
            if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                perror("connect() error\n");
                close(sockfd);
                continue;
            }
            // construct and send the complete hashslot
            CMCData cmc_data;
            cmc_data.set_data_type(CMCData::HASHSLOTINFO);
            HashSlotInfo hs_info;
            new_hash_slot->saveTo(hs_info);
            cmc_data.mutable_hs_info()->CopyFrom(hs_info);
            cmc_data.SerializeToArray(send_buff, BUFF_SIZE_LONG);
            int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
            if (send_size < 0) {
                std::cerr << "Send the complete hashslot failed!\n";
            }
            // wait for HASHSLOTUPDATEACK
            if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
            } else {
                std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
            }
            close(sockfd);
            // ---------------- 3 ----------------
            // sequentially connect each old cache server and send the hashslot CHANGE info
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cur < new_hash_slot->numNodes() - 1; ++cur, ++cache_it) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                const char* cache_ip = cache_it->ip().c_str();  // cache server's ip
                const u_int16_t cache_port = cache_it->port();  // cache server's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect the old node
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means the current node has already died
                    continue;   // continue to deal with the next node
                }
                // construct and send the hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::ADDCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
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
            // ---------------- 4 ----------------
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                m->hash_slot_.reset(new_hash_slot.release());  // replace the old hashslot
            }
            // ---------------- 5 ----------------
            // sequentially connect each client and send the hashslot CHANGE info
            std::unordered_set<Addr, addrHasher> client_links;
            {
                std::lock_guard<std::mutex> lock(m->client_links_mutex_);
                client_links = m->client_links_;  // copy intialise a local replica of the client list
            }
            for (auto &client : client_links) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                // record the client address
                const char* client_ip = client.first.c_str();  // client's ip
                const u_int16_t client_port = client.second;  // client's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means the current client has already left
                    continue;   // continue to deal with the next client
                }
                // construct and send hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::ADDCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // Debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                std::cout << "After SerializeToArray4: " << strlen(send_buff) << std::endl;
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
            std::cout << "TASK_ADD Completed!" << std::endl;
        } else if (task.first == TASK_LOST) {
            // 1. remove the lost node from the cache list
            // 2. sequentially notify each remaining node of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            // 3. replace the old hashslot
            // 4. notify all clients of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            std::unique_ptr<HashSlot> new_hash_slot;
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_));
            }
            // ---------------- 1 ----------------
            // remove the lost node from the cache list
            Addr addr = {task.second.first, task.second.second};
            new_hash_slot->remCacheNode(CacheNode(addr.first, addr.second));
            // ---------------- 2 ----------------
            // sequentially notify each remaining node of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cache_it != new_hash_slot->cend(); ++cache_it) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                const char* cache_ip = cache_it->ip().c_str();  // cache server's ip
                const u_int16_t cache_port = cache_it->port();  // cache server's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means that the current node has already died
                    continue;   // continue to deal with the next node
                }
                // construct and send the hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                std::cout << cmc_data.DebugString() << std::endl;                   // Debug string
                std::cout << "Data Size: " << cmc_data.ByteSizeLong() << std::endl;
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                std::cout << "After SerializeToArray5: " << strlen(send_buff) << std::endl;
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
            // ---------------- 3 ----------------
            // replace the old hashslot
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                m->hash_slot_.reset(new_hash_slot.release());
            }
            // ---------------- 4 ----------------
            // notify all clients of the CHANGE of the hashslot
            std::unordered_set<Addr, addrHasher> client_links;
            {
                std::lock_guard<std::mutex> lock(m->client_links_mutex_);
                client_links = m->client_links_;  // copy intialise a local replica of the client list
            }
            for (auto &client : client_links) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                // record the client addess
                const char* client_ip = client.first.c_str();  // client's ip
                const u_int16_t client_port = client.second;  // client's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means the current client has already left
                    continue;   // continue to deal with the next client
                }
                // construct and send the hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
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
            // 1. remove the leaving node from the node list
            // 2. sequentially notify each remaining nodes of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            // 3. notify the leaving node of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            // 4. notify the leaving node of OFFLINEACK, and add it to the blacklist, ignoring unexpected heartbeats from the same {ip:port}
            // 5. remove the already left node from the list of all living cache servers
            // 6. replace the old hashslot
            // 7. sequentially notify each client of the CHANGE of the hashslot
            std::unique_ptr<HashSlot> new_hash_slot;
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                new_hash_slot = std::make_unique<HashSlot>(*(m->hash_slot_));
            }
            // ---------------- 1 ----------------
            Addr addr = {task.second.first, task.second.second};
            new_hash_slot->remCacheNode(CacheNode(addr.first, addr.second));
            // ---------------- 2 ----------------
            int cur = 0;
            for (auto cache_it = new_hash_slot->cbegin(); cache_it != new_hash_slot->cend(); ++cache_it) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                const char* cache_ip = cache_it->ip().c_str();  // cache server's ip
                const u_int16_t cache_port = cache_it->port();  // cache server's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect
                if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means the current node has already died
                    continue;  // continue to deal with the next node
                }
                // construct and send the hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
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
            // ---------------- 3 ----------------
            // notify the leaving node of the CHANGE of the hashslot and wait for HASHSLOTUPDATEACK
            char send_buff[BUFF_SIZE];
            bzero(send_buff, BUFF_SIZE);
            const char* cache_ip = addr.first.c_str();  // the leaving cache server's ip
            const u_int16_t cache_port = addr.second;  // the leaving cache server's port
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
            // create a socket
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket() error\n");
            }
            // block connect
            if (connect(sockfd, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
                perror("connect() error\n");
                close(sockfd);
            }
            // construct and send the hashslot CHANGE info
            CMCData cmc_data;
            cmc_data.set_data_type(CMCData::HASHSLOTINFO);
            auto hs_info_ptr = cmc_data.mutable_hs_info();
            hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
            auto cache_info = hs_info_ptr->mutable_cache_node();
            cache_info->set_ip(addr.first);
            cache_info->set_port(addr.second);
            cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
            int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
            if (send_size < 0) {
                std::cout << "Send hashslot change failed!" << std::endl;
            }
            // wait for HASHSLOTUPDATEACK
            if (checkAckInfo(sockfd, AckInfo::HASHSLOTUPDATEACK)) {
                std::cout << "HASHSLOTUPDATEACK from cache server received!" << std::endl;
            } else {
                std::cout << "HASHSLOTUPDATEACK from cache server receive error!" << std::endl;
            }
            // ---------------- 4 ----------------
            std::cout << "Sending OFFLINEACK..." << std::endl;
            char testbuf[BUFF_SIZE];
            bzero(testbuf, BUFF_SIZE);
            AckInfo ack_info;
            ack_info.set_ack_type(AckInfo::OFFLINEACK);
            ack_info.set_ack_status(AckInfo::OK);
            cmc_data.Clear();
            cmc_data.set_data_type(CMCData::ACKINFO);
            cmc_data.mutable_ack_info()->CopyFrom(ack_info);
            cmc_data.SerializeToArray(testbuf, BUFF_SIZE);
            std::cout << cmc_data.DebugString() << std::endl;
            send_size = send(sockfd, testbuf, cmc_data.ByteSizeLong(), 0);
            if (send_size < 0) {
                std::cout << "Send OFFLINEACK failed!" << std::endl;
            }
            // add the permitted-to-leave node to the blacklist
            {
                std::lock_guard<std::mutex> lock(m->blacklist_mutex_);
                m->blacklist_[{addr.first, addr.second}] = std::time(0);
            }
            // ---------------- 5 ----------------
            {
                std::lock_guard<std::mutex> lock(m->cache_links_mutex_);
                m->cache_links_.erase({addr.first, addr.second});
            }
            // MUST NOT immediately close the socket, register an EPOLL event instead
            epoll_event ev;
            bzero(&ev, sizeof(ev));
            ev.events = EPOLLET;  // triggered when the socket is closed, use EPOLLLET only
            ev.data.fd = sockfd;
            if (epoll_ctl(m->m_epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
                std::cerr << "register event error\n";
                return;
            }
            // ---------------- 6 ----------------
            {
                std::lock_guard<std::mutex> lock(m->hash_slot_mutex_);
                m->hash_slot_.reset(new_hash_slot.release());  // replace the old hashslot
            }
            std::unordered_set<Addr, addrHasher> client_links;
            // ---------------- 7 ----------------
            {
                std::lock_guard<std::mutex> lock(m->client_links_mutex_);
                client_links = m->client_links_;  // copy intialise a local replica of the list of clients
            }
            // connect each client and send the hashslot CHANGE info
            for (auto &client : client_links) {
                char send_buff[BUFF_SIZE];
                bzero(send_buff, BUFF_SIZE);
                // record the client addess
                const char* client_ip = client.first.c_str();  // client's ip
                const u_int16_t client_port = client.second;  // client's port
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
                // create a socket
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("socket() error\n");
                }
                // block connect
                if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(sockaddr_in)) < 0) {
                    perror("connect() error\n");
                    close(sockfd);
                    // error means the current client has already left
                    continue;  // continue to deal with the next client
                }
                // construct and send the hashslot CHANGE info
                CMCData cmc_data;
                cmc_data.set_data_type(CMCData::HASHSLOTINFO);
                auto hs_info_ptr = cmc_data.mutable_hs_info();
                hs_info_ptr->set_hashinfo_type(HashSlotInfo::REMCACHE);
                auto cache_info = hs_info_ptr->mutable_cache_node();
                cache_info->set_ip(addr.first);
                cache_info->set_port(addr.second);
                cmc_data.SerializeToArray(send_buff, BUFF_SIZE);
                int send_size = send(sockfd, send_buff, cmc_data.ByteSizeLong(), 0);
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
    }
}

// Utility functions
bool checkAckInfo(int sockfd, int ackType)
{
    char recv_buff[BUFF_SIZE];
    bzero(recv_buff, BUFF_SIZE);
    int recv_size = recv(sockfd, recv_buff, BUFF_SIZE, 0);
    if (recv_size <= 0) {
        std::cerr << "ACK recv() error!" << std::endl;
    } else {
        // std::cout << "received: " << recv_size << " Byte" << std::endl;
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
 