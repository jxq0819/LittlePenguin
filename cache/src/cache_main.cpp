// "gitignore test"
#include <stdlib.h>

#include <iostream>

#include "CacheServer.h"
#include "global_extern.h"
#include "unistd.h"

// 全局变量
static struct sockaddr_in master_addr;  // master地址信息，全局静态变量
extern bool offline_applying;         // cache下线申请触发标志变量，声明为来自的外部全局变量
extern bool offline_applied;          // 管理员是否申请过下线

// 下线申请信号(SIGQUIT)回调函数声明，向master发送OFFLINE数据包
void offlineApply(int signal_num) {
    if (signal_num == SIGQUIT) {
        std::cout << "signal_num == SIGQUIT" << std::endl;
        // 修改offline_apply_flag下线申请标志全局变量为true
        offline_applying = true;
        offline_applied = true;     // 置为true说明曾经确实申请过下线，后续此变量状态不会变动
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s master_ip master_port\n", argv[0]);
        return -1;
    }

    CacheServer myServer;

    /* ---------------------注册master主机地址信息--------------------- */
    const char* master_ip = argv[1];              // master主机ip
    const u_int16_t master_port = atoi(argv[2]);  // master主机port
    if (master_port <= MIN_PORT || master_port >= MAX_PORT) {
        std::cout << "port error\n";
        return -1;
    }
    bzero(&master_addr, 0);
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(master_port);
    if (inet_pton(AF_INET, master_ip, &master_addr.sin_addr.s_addr) < 0) {
        perror("inet_pton() error\n");
        return -1;
    }
    /* ---------------------与master建立持久连接发送心跳--------------------- */
    sockaddr_in cache_addr;
    bzero(&cache_addr, sizeof(cache_addr));
    myServer.beginHeartbeatThread(master_addr, cache_addr);  // 启动心跳线程

    /* ----------注册SIGQUIT信号，管理员按（Ctrl + \）发送一个SIGQUIT信号，并向master申请下线----------- */
    struct sigaction offline_act;
    offline_act.sa_handler = offlineApply;  // 注册回调函数
    sigemptyset(&offline_act.sa_mask);
    offline_act.sa_flags = 0;
    sigaction(SIGQUIT, &offline_act, 0);  // 当收到进程收到系统发来的SIGQUIT信号后，调用offlineApply处理

    /* ---------------------绑定自身主机信息和设置监听上限--------------------- */
    std::cout << "-------------------------------------------" << std::endl;
    // int myport = atoi(argv[1]);  // cache自己的端口号
    int myport;                     // cache自己的端口号
    std::string myip;
    while (!ntohs(cache_addr.sin_port)) { /* empty body */ }    // wait for the connection to the master to be established
    myport = ntohs(cache_addr.sin_port);
    myip = inet_ntoa(cache_addr.sin_addr);
    std::cout << "myport = " << myport << std::endl;
    std::cout << "myip = " << myip << std::endl;
    std::cout << "Cache program started..." << std::endl;

    myServer.registerLocalAddr(cache_addr);     // 注册CacheServer类myServer对象中m_serv_addr的地址信息(ip&port)

    if (!myServer.bindAndListen(myport)) {
        std::cout << "listen error !";
        return 0;
    }
    std::cout << "start listening on port: " << myport << std::endl;
    std::cout << "Enter [Ctrl + \\] to apply for offline." << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    /* ---------------------cache server启动服务,开始epoll监听--------------------- */
    if (!myServer.startService()) {
        std::cout << "The cache server process exited.\n";
    }

    return 0;
}
