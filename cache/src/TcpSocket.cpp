#include "TcpSocket.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "user_data.h"

/* 封装socket的基本操作 */

TcpSocket::TcpSocket(int maxWaiter) {
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0) {
        std::cout << "create socket failed\n";
        return;
    }
    m_maxWaiter = maxWaiter;
}

TcpSocket::~TcpSocket() {
    if (m_sockfd >= 0) {
        close(m_sockfd);
    }
}

bool TcpSocket::bindPort(int _port) {
    if (_port < MIN_PORT || MAX_PORT > MAX_PORT) {
        return false;
    }

    // 设置端口复用，这样断开连接后，不用等待2 MSL时间也不用更换端口号即可重新使用该端口号
    int opt = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    m_local_addr.sin_family = AF_INET;
    m_local_addr.sin_port = htons(_port);
    m_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(m_sockfd, (struct sockaddr *)&m_local_addr, sizeof(m_local_addr)) < 0) {
        return false;
    }
    return true;
}

bool TcpSocket::setServerInfo(const std::string &ip, int _port) {
    auto IP = ip.data();
    bzero(&m_local_addr, sizeof(m_local_addr));
    if (inet_pton(AF_INET, IP, &m_local_addr.sin_addr.s_addr) < 0) {
        return false;
    }
    if (_port < MIN_PORT || MAX_PORT > 65535) {
        return false;
    }
    m_local_addr.sin_port = htons(_port);
    m_local_addr.sin_family = AF_INET;

    return true;
}

bool TcpSocket::connectToHost() {
    if (connect(m_sockfd, (struct sockaddr *)&m_local_addr, sizeof(m_local_addr)) < 0) {
        return false;
    }
    return true;
}

bool TcpSocket::listenOn() {
    if (listen(m_sockfd, m_maxWaiter) < 0) {
        return false;
    }
    return true;
}

unsigned int TcpSocket::getPort() const {
    auto port = m_local_addr.sin_port;
    return static_cast<unsigned int>(port);
}
