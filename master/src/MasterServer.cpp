#include "MasterServer.h"

#include "cmcdata.pb.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
// 8 Dec 2021
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctime>
// #include <fstream>
#include <iostream>
#include <string>
#include <ctime>
// #include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <chrono>

// MasterServer构造函数，设置客户端最大连接个数
MasterServer::MasterServer(int maxWaiter) : TcpServer(maxWaiter) {
    // 线程池预先开启8个线程
    threadPool = std::make_unique<ThreadPool>(8);
    // 7 Dec 2021
    hash_slot_ = std::make_unique<HashSlot>();    // New empty hash_slot
}

void MasterServer::init() {
    // TODO: Start a heartbeat-monitoring thread and a hashslot management thread
    std::thread heartbeat_service(startHeartbeartService, this);
    heartbeat_service.detach(); // 是这样写吗？
    std::thread hashslot_service(startHashslotService, this);
    hashslot_service.detach();
}

void MasterServer::newConnection() {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "New connection" << std::endl;
    // 建立新连接在主线程
    sockaddr_in new_addr;
    socklen_t len = sizeof(new_addr);
    int connfd = accept(m_listen_sockfd, (sockaddr*) &new_addr, &len);  // 获取与客户端相连的fd，但不保存客户端地址信息
    if (connfd < 0) {
        // throw std::runtime_error("accept new connection error\n");
        std::cout << "accept new connection error\n";
    }
    // Record the AddrPair {ip:port} 
    char p_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(new_addr.sin_addr.s_addr), p_addr, sizeof(p_addr));    // Convert IP address from numeric to presentation
    links_.insert({std::string(p_addr), new_addr.sin_port});                    // Add the Addr of the new connection to [links_]
    std::cout << p_addr << ":" << new_addr.sin_port << " has connected" << std::endl;

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
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "existing connection\n";
    // 处理已经存在客户端的请求在子线程处理
    threadPool->enqueue([this, event_i]() {  // lambda统一处理即可
        if (this->m_epollEvents[event_i].events & EPOLLRDHUP) {
            // 有客户端事件发生，但缓冲区没数据可读，说明主动断开连接
            // Remove the AddrPair {ip:port} from links_
            sockaddr_in left_addr;
            socklen_t len = sizeof(left_addr);
            getpeername(this->m_epollEvents[event_i].data.fd, (sockaddr*) &left_addr, &len);
            // Record the AddrPair {ip:port} 
            char p_addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(left_addr.sin_addr.s_addr), p_addr, sizeof(p_addr));   // convert IP address from numeric to presentation
            links_.erase({std::string(p_addr), left_addr.sin_port});
            client_links_.erase({std::string(p_addr), left_addr.sin_port});             // remove if present, otherwise do nothing
            cache_links_.erase({std::string(p_addr), left_addr.sin_port});              // remove if present, otherwise do nothing
            std::cout << p_addr << ":" << left_addr.sin_port << " has left" << std::endl;
            // 将其从epoll树上摘除
            if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_epollEvents[event_i].data.fd, NULL) < 0) {
                throw std::runtime_error("delete client error\n");
            }
            std::cout << "a client left" << std::endl;
        } else if (this->m_epollEvents[event_i].events & EPOLLIN) {
            // 如果与客户端连接的该套接字的输入缓存中有收到数据
            char buf[MAX_BUFFER];
            memset(buf, 0, MAX_BUFFER);
            int recv_size = recv(m_epollEvents[event_i].data.fd, buf, MAX_BUFFER, 0);
            std::cout << "received: " << recv_size << " Byte" << std::endl;

            if (recv_size <= 0) {
                throw std::runtime_error("recv() error \n");
            } else {
                std::cout << "Receive message from client fd: " << m_epollEvents[event_i].data.fd << std::endl;
                CMCData recv_cmc_data;
                recv_cmc_data.ParseFromArray(buf, recv_size);
                // bzero(buf, sizeof(buf));
                std::string debug_str = recv_cmc_data.DebugString();
                //
                std::cout << debug_str << std::endl;
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                getpeername(this->m_epollEvents[event_i].data.fd, (sockaddr*) &addr, &len);
                // Record the AddrPair {ip:port} 
                char p_addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr.sin_addr.s_addr), p_addr, sizeof(p_addr));    // Convert IP address from numeric to presentation

                switch (recv_cmc_data.data_type()) {
                    case CMCData::COMMANDINFO:
                        std::cout << "Command received" << std::endl;
                        //
                        switch (recv_cmc_data.cmd_info().cmd_type()) {
                            case CommandInfo::GETSLOT:
                                std::cout << "Command GETSLOT received" << std::endl;
                                client_links_.insert({std::string(p_addr),addr.sin_port});  // Add the current link to [client_link_]
                                //
                                break;
                            // Other cases
                            case CommandInfo::OFFLINE:
                                std::cout << "OFFLINE request received" << std::endl;
                                client_links_.erase({std::string(p_addr),addr.sin_port});  // Remove the current link from [client_link_]
                                break;
                            default:;
                        }
                    break;
                    case CMCData::HEARTINFO:
                        std::cout << "Heartbeat from " << p_addr << ":" << addr.sin_port << std::endl;
                        HeartbeatInfo ht_info;
                        ht_info.time = recv_cmc_data.ht_info().cur_time();
                        ht_info.status = recv_cmc_data.ht_info().cache_status();
                        if (cache_links_.count({std::string(p_addr),addr.sin_port})) {
                            // TODO: Update Heartbeat Info
                        } else {
                            cache_links_[{std::string(p_addr),addr.sin_port}] = ht_info;        // Heartbeat from a new cache server
                            task_queue_.Push({TASK_ADD, {std::string(p_addr),addr.sin_port}});  // 扩容
                        }
                        break;
                    // Other cases
                    default:;
                }

                // 回复客户端：我收到你的消息了
                const char *msg = "Received your message!\n";
                int ret = send(m_epollEvents[event_i].data.fd, msg, strlen(msg), 0);

                if (ret < 0) {
                    // throw std::runtime_error("error in send()\n");
                    std::cout << "error on send()\n";
                    return;
                }

            }
        } else {  // 未知错误
            // throw std::runtime_error("unknown error");
            std::cout << "unknown error\n";
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
        time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());  // Get current time in seconds
        // Check if all cache servers are ok, should the current time be obtained in the loop ?
        bool all_good = true;
        for (auto it = m->cache_links_.begin(); it != m->cache_links_.end(); ++it) {
            if (t - it->second.time > 1) {    // Heartbeat lost
                std::cout << "lost cache server " << it->first.first << ":" << it->first.second << "\n";
                m->task_queue_.Push({TASK_ADD, it->first});     // Add a task to remove the lost cache server
                m->cache_links_.erase(it);                      // Remove the lost cache server from cache_links_
                all_good = false;
            }
        }
        if (all_good) { 
            std::cout << "All cache servers are good\n";
        }
        sleep(1);
    }
}

void MasterServer::startHashslotService(MasterServer *m)
{
    // while (1) {
    //     std::cout << "hashslot thread running!\n";
    //     sleep(5);
    // }
    std::cout << "Started hashslot thread!\n";
    while (1) {
        if (m->task_queue_.Size()) {
            
        }
        sleep(1);
    }
}