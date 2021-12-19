#pragma once
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "TcpSocket.h"
#include "user_data.h"

/* 封装一个TcpServer类型 */

class TcpServer {
  public:
    explicit TcpServer(int maxWaiter = 10);  // 默认客户端最大连接个数为10

    bool bindAndListen(int _port = 6666);  // 默认端口为6666

    // 将(socket)文件描述符设置成非阻塞
    int setnonblocking(int fd);

    // 处理新的连接函数，此处为纯虚函数，具体由子类实现
    virtual void newConnection() = 0;
    // 处理已经存在的连接的函数，此处为纯虚函数，具体由子类实现
    virtual void existConnection(int sockfd) = 0;

    // 在这里进入事件的主循环，可以自行设置时间，默认为epoll_wait堵塞等待事件发生
    bool startService(int timeout = -1);

    // 注册本机所使用的ip地址和port
    void registerLocalAddr(sockaddr_in& myaddr);

  protected:
    struct sockaddr_in m_serv_addr;            // 服务器信息
    std::unique_ptr<TcpSocket> m_tcpSocket;  // unique_ptr智能指针，指向一个TcpSocket对象

    epoll_event m_epollEvents[MAX_EVENTS];  // epoll事件队列
    int m_epfd;                             // epoll的fd
    int m_listen_sockfd;
    
    bool m_epoll_is_ready;  // epoll监听就绪状态标识，发心跳前检查此变量是否为true，避免还没准备好监听master就发消息过来了
};
