#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

#include "HashSlot.h"
#include "cmcdata.pb.h"
#include "command.h"

#define MAX_BUF_SIZE 212992

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s master_ip master_port\n", argv[0]);
        return -1;
    }

    /* ---------------------全局变量区--------------------- */
    char cache_ip[16];         // 目的主机ip
    u_int16_t cache_port = 0;  // 目的主机port
    HashSlot hashslot;         // 本地哈希槽对象
    // char read_buf[BUFSIZ];            //普通大小的缓存
    char recv_buf_max[MAX_BUF_SIZE];  // 为了接受类似于hashslot型的大字节数据，保险起见就开了这么大的空间

    /* ---------------------------------------------------- */

    /* -----------------------本地调试代码（后续不需要这段代码）----------------------------- */
    // HashSlotInfo hashslot_info;  //哈希槽信息
    // hashslot.addCacheNode(CacheNode("Cache1", "127.0.0.1", 5001));
    // hashslot.addCacheNode(CacheNode("Cache2", "127.0.0.2", 5002));
    // hashslot.saveTo(hashslot_info);  // 生成本地哈希槽信息
    // CMCData hashslot_data;
    // hashslot_data.set_data_type(CMCData::COMMANDINFO);
    // auto hs_info_ptr = hashslot_data.mutable_hs_info();
    // hs_info_ptr->CopyFrom(hashslot_info);
    // //
    // string hashslot_data_str = hashslot_data.DebugString();
    // cout << hashslot_data_str << endl;  // Print the debugging string

    // CommandInfo testcmdinfo;
    // testcmdinfo.set_cmd_type(CommandInfo::SET);

    /* ---------------------------------------------------------------------------------------*/

    // 注册master主机地址信息
    const char* master_ip = argv[1];              // master主机ip
    const u_int16_t master_port = atoi(argv[2]);  // master主机port
    if (master_port <= 1024 || master_port >= 65535) {
        perror("master_port error");
        return -1;
    }
    struct sockaddr_in master_addr;
    bzero(&master_addr, 0);
    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(master_port);
    if (inet_pton(AF_INET, master_ip, &master_addr.sin_addr.s_addr) < 0) {
        perror("inet_pton() error\n");
        return -1;
    }

    // 创建与master相连的客户端套接字文件描述符
    int socketfd_to_master = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd_to_master < 0) {
        perror("socket() error\n");
        return -1;
    }

    // 阻塞连接master
    if (connect(socketfd_to_master, (struct sockaddr*)&master_addr, sizeof(sockaddr_in)) < 0) {
        perror("connect() error\n");
        close(socketfd_to_master);
        return -1;
    }

    cout << "connect master server success!" << endl;
    cout << "enter [Q]/[q] to quit." << endl;

    /* --------------- 和master连接成功的第一件事就是拉取哈希槽信息（GETSLOT） --------------- */
    if (get_slot(socketfd_to_master, hashslot) == false) {
        cout << "get_slot() fail." << endl;
        return -1;
    } else {
        cout << "get_slot() successful." << endl;
    }

    /* --------------- poll I/O复用监测不同事件的发生 --------------- */
    pollfd fd_array[2];  //监听事件数组
    // --------注册第一种事件：标准输入引发的事件
    fd_array[0].fd = STDIN_FILENO;  // 读取标准输入
    fd_array[0].events = POLLIN;
    fd_array[0].revents = 0;  // 初始化

    // --------注册第二种事件：网络套接字事件
    fd_array[1].fd = socketfd_to_master;  // 处理socket事件
    // 对端连接断开触发的poll事件会包含：EPOLLIN | EPOLLRDHUP，这ET模式下非常有用
    fd_array[1].events = POLLIN | POLLRDHUP;
    fd_array[1].revents = 0;

    // poll循环监听io事件
    while (1) {
        int ret = poll(fd_array, 2, -1);  // 这里阻塞等待事件发生
        if (ret < 0) {
            perror("poll() error\n");
            return -1;
        }

        // 发生对端（服务端）断开关闭事件，清空读缓冲数据
        if (fd_array[1].revents & POLLRDHUP) {
            bzero(recv_buf_max, sizeof(recv_buf_max));
            printf("master close the connection\n");
            break;
        }

        /* -------- 发生对端套接字输入事件(一般是来自master主动哈希槽更新通知)，遇到这种情况，需要更新本地哈希槽 --------- */
        if (fd_array[1].revents & POLLIN) {
            // 接受数据
            bzero(recv_buf_max, sizeof(recv_buf_max));
            int recv_size = recv(fd_array[1].fd, recv_buf_max, sizeof(recv_buf_max), 0);
            CMCData recv_data;
            recv_data.ParseFromArray(recv_buf_max, recv_size);
            // 处理数据
            // string recv_info_str = recv_data.DebugString();
            // cout << recv_info_str << endl;
            switch (recv_data.data_type()) {
                // 如果真的是哈希槽信息包，那就更新哈希槽
                case CMCData::HASHSLOTINFO:

                    hashslot.restoreFrom(recv_data.hs_info());

                    break;
                default:
                    break;
            }
        }

        /* -------------------- 发生本机标准输入事件 -------------------- */
        if (fd_array[0].revents & POLLIN) {
            std::string client_input_str;  // 客户端命令
            getline(std::cin, client_input_str);
            istringstream istr(client_input_str);
            string word;
            vector<string> word_v;
            while (istr >> word) {
                word_v.push_back(word);
            }
            string command, param_1, param_2;
            int param_count = word_v.size();
            switch (param_count) {
                case 0:
                    continue;
                case 1: {
                    command = word_v[0];
                    break;
                }
                case 2: {
                    command = word_v[0];
                    param_1 = word_v[1];
                    break;
                }
                case 3: {
                    command = word_v[0];
                    param_1 = word_v[1];
                    param_2 = word_v[2];
                    break;
                }
                default: {
                    cout << "too many arguments!" << endl;
                    continue;
                }
            }
            cout << "Command: " << command << endl;
            cout << "Param1: " << param_1 << endl;
            cout << "Param2: " << param_2 << endl;

            /* ------------- 执行命令 ------------- */
            CMCData my_cmc_data;
            if ((command == "QUIT" || command == "quit") && param_count == 1) {
                cout << "The client has been close!" << endl;
                break;  // break while(1), close(socketfd_to_master), return 0;
            } else if ((command == "GET" || command == "get") && param_count == 2) {
                my_cmc_data = MakeCommandData(CommandInfo::GET, param_1, "");
            } else if ((command == "SET" || command == "set") && param_count == 3) {
                my_cmc_data = MakeCommandData(CommandInfo::SET, param_1, param_2);
            } else if ((command == "DEL" || command == "del") && param_count == 2) {
                my_cmc_data = MakeCommandData(CommandInfo::DEL, param_1, "");
            }
            strcpy(cache_ip, hashslot.getCacheAddr(param_1).first.c_str());  // 从hashslot中查询cache地址
            cache_port = hashslot.getCacheAddr(param_1).second;
            cout << "will SendCommandData" << endl;
            CMCData result_data;                                                           // 访问的结果数据包
            if (SendCommandData(my_cmc_data, cache_ip, cache_port, result_data) == false)  // 发送命令数据包给cache
                cout << "sendCommandData fail." << endl;

            // 上面结果数据包result_data收到相关信息后，这里是需要以用户的视角把结果打印到屏幕的
            // 解析数据（主要是查看有没有想要查询的结果），并打印结果
            /* ------------- TODO ------------ */
            // 此处先把数据包信息打印出来
            string debug_str = result_data.DebugString();
            cout << debug_str << endl;
            cout << "DebugString() end!" << endl;
        }
    }
    close(socketfd_to_master);  // 关闭客户端套接字，客户端退出
    return 0;
}