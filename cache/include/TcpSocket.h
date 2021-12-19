#pragma once

#include <netinet/ip.h>

#include <string>

/* 封装socket的基本操作 */

class TcpSocket {
  public:
    explicit TcpSocket(int maxWaiter = 10);

    ~TcpSocket();

    // 封装socket中的bind()函数，bindPort(int _port) 服务器端调用此函数，用于绑定自己的地址信息(ip & port)
    bool bindPort(int _port);

    // setServerInfo() 客户端调用此函数，用于绑定目标服务器的地址信息
    bool setServerInfo(const std::string &ip, int _port);

    bool listenOn();  // 对应于socket中的listen函数

    bool connectToHost();  // 对应于socket中的connect函数，供客户端程序调用

    unsigned int getPort() const;

    inline int getSockFD() const { return m_sockfd; }

  private:
    int m_sockfd{-1};
    struct sockaddr_in m_local_addr;  // 客户端的socket表示需要连接的远程服务器地址信息，服务器的socket表示自己的地址信息
    int m_maxWaiter{10};           // 同时与服务器建立连接的上限数
};
