#include <stdlib.h>

#include <iostream>

#include "CacheServer.h"
#include "unistd.h"

int main(int argc, char* argv[]) {
    if (argc <= 3) {
        printf("usage: %s cache_port master_ip master_port\n", argv[0]);
        return -1;
    }

    // cache自己的端口号
    int myport = atoi(argv[1]);

    CacheServer myServer;

    /* ---------------------与master建立持久连接发送心跳--------------------- */
    // 注册master主机地址信息
    const char* master_ip = argv[2];              // master主机ip
    const u_int16_t master_port = atoi(argv[3]);  // master主机port
    if (master_port <= MIN_PORT || master_port >= MAX_PORT) {
        std::cout << "port error\n";
        return -1;
    }
    struct sockaddr_in master_addr;  // master地址信息
    bzero(&master_addr, 0);
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(master_port);
    if (inet_pton(AF_INET, master_ip, &master_addr.sin_addr.s_addr) < 0) {
        perror("inet_pton() error\n");
        return -1;
    }
    myServer.beginHeartbeatThread(master_addr);  // 启动心跳线程

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Cache program started..." << std::endl;
    if (!myServer.bindAndListen(myport)) {
        std::cout << "listen error !";
        return 0;
    }
    std::cout << "start listening on port: " << myport << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    /* ---------------------cache server启动服务,开始epoll监听--------------------- */
    if (!myServer.startService()) {
        std::cout << "start service error\n";
        return 0;
    }
    return 0;
}