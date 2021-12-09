#include "TcpServer.h"

#include <strings.h>

#include <iostream>

// 构造函数，成员变量初始化
TcpServer::TcpServer(int maxWaiter) {
    // maxWaiter用在：listen(m_sockfd, m_maxWaiter)，用于设置最大监听数量，默认10
    m_tcpSocket = std::make_unique<TcpSocket>(maxWaiter); // 新建一个TcpSocket对象，返回一个unique_ptr赋值给m_tcpSocket
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1024); // 1024端口是动态端口的开始，先暂时设置为1024，后续通过bind更改
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    m_listen_sockfd = m_tcpSocket->getSockFD();
    m_epfd = epoll_create1(0); // Same as epoll_create() but with an FLAGS parameter.
}

// 服务器端：绑定(bind)地址信息并设置最大监听(listen)个数
// 默认参数端口6666，默认监听个数为10个
bool TcpServer::bindAndListen(int _port) {
    return m_tcpSocket->bindPort(_port) && m_tcpSocket->listenOn();
}

// 服务器服务函数
bool TcpServer::startService(int timeout) { // timeout默认声明了timeout = -1，即默认阻塞等待
    // 新建一个事件结构体，并注册事件：
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.data.fd = m_listen_sockfd;
    ev.events = EPOLLIN | EPOLLET;  // 新来的连接
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_listen_sockfd, &ev) < 0) {
        return false;
    }

    // 服务器的监听循环
    while (1) {
        // 等待事件发生
        int event_cnt = epoll_wait(m_epfd, m_epollEvents, MAX_EVENTS, timeout);
        if (event_cnt < 0) return false;
        // 事件发生，遍历m_epollEvents队列中的事件，并分类处理
        for (int i = 0; i < event_cnt; ++i) {
            // 对于新连接请求，则调用新连接处理函数
            if (m_epollEvents[i].data.fd == m_listen_sockfd) {
                newConnection();
            } else {
            // 处理已经存在的连接，在线程池中启动一个线程进行处理
                existConnection(i);
            } 
        }
    }

    return true;
}
