#include <stdlib.h>

#include <iostream>

#include "CacheServer.h"
#include "unistd.h"

// 全局变量
static struct sockaddr_in master_addr;  // master地址信息，全局静态变量

// 下线申请信号(SIGQUIT)回调函数声明，向master发送OFFLINE数据包
void offlineApply(int signal_num);

int main(int argc, char* argv[]) {
    if (argc <= 3) {
        printf("usage: %s cache_port master_ip master_port\n", argv[0]);
        return -1;
    }

    CacheServer myServer;

    /* ---------------------注册master主机地址信息--------------------- */
    const char* master_ip = argv[2];              // master主机ip
    const u_int16_t master_port = atoi(argv[3]);  // master主机port
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
    myServer.beginHeartbeatThread(master_addr);  // 启动心跳线程

    /* ----------注册SIGQUIT信号，管理员按（Ctrl + \）发送一个SIGQUIT信号，并向master申请下线----------- */
    struct sigaction offline_act;
    offline_act.sa_handler = offlineApply;  // 注册回调函数
    sigemptyset(&offline_act.sa_mask);
    offline_act.sa_flags = 0;
    sigaction(SIGQUIT, &offline_act, 0);  // 当收到进程收到系统发来的SIGQUIT信号后，调用offlineApply处理

    /* ---------------------绑定自身主机信息和设置监听上限--------------------- */
    std::cout << "-------------------------------------------" << std::endl;
    int myport = atoi(argv[1]);  // cache自己的端口号
    std::cout << "Cache program started..." << std::endl;
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

// 下线申请信号(SIGQUIT)回调函数实现
void offlineApply(int signal_num) {
    if (signal_num == SIGQUIT) {
        cout << "signal_num == SIGQUIT" << endl;
        /* ---------------------与master建立临时连接用于发送下线申请--------------------- */
        // 创建与master相连的下线专用套接字文件描述符
        int offline_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (offline_sock < 0) {
            perror("offline socket() error\n");
            return;
        }
        // 阻塞连接master
        if (connect(offline_sock, (struct sockaddr*)&master_addr, sizeof(sockaddr_in)) < 0) {
            perror("offline connect() error\n");
            close(offline_sock);
            return;
        }

        cout << "offline connection to master successful!" << endl;

        // 制作下线申请包并序列化
        CommandInfo offline_cmd_info;
        offline_cmd_info.set_cmd_type(CommandInfo::OFFLINE);
        CMCData offline_cmc_data;
        offline_cmc_data.set_data_type(CMCData::COMMANDINFO);
        auto cmd_info_ptr = offline_cmc_data.mutable_cmd_info();
        cmd_info_ptr->CopyFrom(offline_cmd_info);

        int data_size = offline_cmc_data.ByteSizeLong();
        char offline_send_buff[BUFSIZ];
        offline_cmc_data.SerializeToArray(offline_send_buff, data_size);

        // 向master端发送下线申请包
        int send_size = send(offline_sock, offline_send_buff, data_size, 0);

        if (send_size > 0)
            cout << "Heartbeat thread started successfully!" << send_size << endl;
        else if (send_size < 0) {
            std::cout << "Offline apply sending failed!" << std::endl;
            return;
        }
    }
}