#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

#include "HashSlot.h"
#include "cmcdata.pb.h"
#include "command.h"

#define MAX_BUF_SIZE 102400
#define BUF_SIZE 2048
#define KV_LEN 64

std::atomic<bool> test_stop(false);
static HashSlot hashslot;
std::mutex hashslot_mutex;

void generateRandomKeyValuePairs(std::vector<std::pair<std::string, std::string>> &kv_vec);
void startTest(int batch_size, int sleep_time);
void stopTest(int signal_num) {
    if (signal_num == SIGQUIT) {
        std::cout << "Test Aborted" << std::endl;
        // 修改offline_apply_flag下线申请标志全局变量为true
        test_stop = true;
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s master_ip master_port\n", argv[0]);
        return -1;
    }

    /* ---------------------全局变量区--------------------- */
    char cache_ip[16];         // 目的主机ip
    u_int16_t cache_port = 0;  // 目的主机port
    // HashSlot hashslot;         // 本地哈希槽对象
    char recv_buf_max[MAX_BUF_SIZE];  // 为了接受类似于hashslot型的大字节数据，保险起见就开了这么大的空间
    char send_buf_max[BUF_SIZE];

    //
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

    // 设置本地端口复用，这样master才能主动联系上client
    int opt = 1;
    setsockopt(socketfd_to_master, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 阻塞连接master
    if (connect(socketfd_to_master, (struct sockaddr*)&master_addr, sizeof(sockaddr_in)) < 0) {
        perror("connect() error\n");
        close(socketfd_to_master);
        return -1;
    }
    std::cout << "connect master server success!" << std::endl;
    std::cout << "enter [QUIT]/[quit] to quit." << std::endl;

    /* --------------- 和master连接成功的第一件事就是拉取哈希槽信息（GETSLOT） --------------- */
    if (get_slot(socketfd_to_master, hashslot) == false) {
        std::cout << "get_slot() fail." << std::endl;
        return -1;
    } else {
        std::cout << "get_slot() successful." << std::endl;
    }

    /* --------------- 绑定本机地址并监听master的请求连接 --------------- */
    int client_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    int connfd_from_master;

    // 设置本地端口复用，这样master才能主动联系上client
    setsockopt(client_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 本机client地址信息
    sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    getsockname(socketfd_to_master, (sockaddr*)&client_addr, &len);

    int myport;                     // client自己的端口号
    myport = ntohs(client_addr.sin_port);
    std::cout << "myport = " << myport << std::endl;

    if (bind(client_listen_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind error");
        return -1;
    }
    if (listen(client_listen_sock, 5) < 0) {
        perror("listen error");
        return -1;
    }

    /* ----------注册SIGQUIT信号，用户按（Ctrl + \）发送一个SIGQUIT信号，停止test ----------- */
    struct sigaction stop_test_act;
    stop_test_act.sa_handler = stopTest;  // 注册回调函数
    sigemptyset(&stop_test_act.sa_mask);
    stop_test_act.sa_flags = 0;
    sigaction(SIGQUIT, &stop_test_act, 0);  // 当收到进程收到系统发来的SIGQUIT信号后，调用stopTest处理

    /* --------------- poll I/O复用监测不同事件的发生 --------------- */
    pollfd fd_array[4];  //监听事件数组
    // --------注册第一种事件：标准输入引发的事件
    fd_array[0].fd = STDIN_FILENO;  // 读取标准输入
    fd_array[0].events = POLLIN;
    fd_array[0].revents = 0;  // 初始化

    // --------注册第二种事件：client请求master这条tcp连接的网络io事件
    fd_array[1].fd = socketfd_to_master;
    // 对端连接断开触发的poll事件会包含：EPOLLIN | EPOLLRDHUP，这ET模式下非常有用
    fd_array[1].events = POLLIN | POLLRDHUP;
    fd_array[1].revents = 0;

    // --------注册第s三种事件：master请求client建立连接的网络io事件
    fd_array[2].fd = client_listen_sock;  // 处理socket事件
    // 对端连接断开触发的poll事件会包含：EPOLLIN | EPOLLRDHUP，这ET模式下非常有用
    fd_array[2].events = POLLIN;
    fd_array[2].revents = 0;

    // poll循环监听io事件
    while (1) {
        int nready = poll(fd_array, 4, -1);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll() error\n");
            return -1;
        }

        // 发生对端（服务端）断开关闭事件，清空读缓冲数据
        if (fd_array[1].revents & POLLRDHUP) {
            bzero(recv_buf_max, sizeof(recv_buf_max));
            printf("master close the connection1\n");
            break;
        }
        /* -------- 刚连上master后，master会发所有节点的hashslot，收到后更新本地哈希槽 --------- */
        /* ------------------ 如果合并1、2连接后这个有用 ----------------------------------- */
        if ((fd_array[1].revents & POLLIN)) {
            // 接受数据
            bzero(recv_buf_max, sizeof(recv_buf_max));
            int recv_size = recv(fd_array[1].fd, recv_buf_max, sizeof(recv_buf_max), 0);
            CMCData recv_data;
            recv_data.ParseFromArray(recv_buf_max, recv_size);
            //
            string recv_info_str = recv_data.DebugString();
            std::cout << recv_info_str << std::endl;
            switch (recv_data.data_type()) {
                // 如果真的是哈希槽信息包，那就更新哈希槽
                case CMCData::HASHSLOTINFO:
                    {
                        std::lock_guard<std::mutex> lock(hashslot_mutex);
                        hashslot.restoreFrom(recv_data.hs_info());
                    }
                    break;
                default:
                    break;
            }
        }

        // master主动请求连接，则accpet，并注册有关事件
        if ((fd_array[2].revents & POLLIN)) {
            if ((connfd_from_master = accept(client_listen_sock, (struct sockaddr*)NULL, NULL)) < 0) {
                printf("accept master fail: %s\n", strerror(errno));
                break;
            }
            fd_array[3].fd = connfd_from_master;
            fd_array[3].events = POLLIN | POLLRDHUP;
        }

        // 发生对端（服务端）断开关闭事件，清空读缓冲数据
        if (fd_array[3].revents & POLLRDHUP) {
            bzero(recv_buf_max, sizeof(recv_buf_max));
            printf("master close the connection2\n");
            // break;
            // reset the event
            fd_array[3].fd = -1;
            fd_array[3].events = POLLIN | POLLRDHUP;
            fd_array[3].revents = 0;
        }
        // master发数据过来了
        if ((fd_array[3].revents & POLLIN)) {
            std::cout << "fd_array[3].revents & POLLIN" << std::endl;
            // 接受数据
            bzero(recv_buf_max, sizeof(recv_buf_max));
            int recv_size = recv(fd_array[3].fd, recv_buf_max, sizeof(recv_buf_max), 0);
            CMCData recv_data;
            recv_data.ParseFromArray(recv_buf_max, recv_size);
            std::cout << recv_data.DebugString() << std::endl;
            std::cout << "DebugString() end" << std::endl; 
            CMCData send_data;
            AckInfo ack_info;

            if (recv_data.data_type() == CMCData::HASHSLOTINFO) {
                if (recv_data.hs_info().hashinfo_type() == HashSlotInfo::ALLCACHEINFO) {
                    std::lock_guard<std::mutex> lock(hashslot_mutex);
                    hashslot.restoreFrom(recv_data.hs_info());
                }
                if (recv_data.hs_info().hashinfo_type() == HashSlotInfo::ADDCACHE) {
                    string add_node_ip = recv_data.hs_info().cache_node().ip();
                    int add_node_port = recv_data.hs_info().cache_node().port();
                    std::lock_guard<std::mutex> lock(hashslot_mutex);
                    hashslot.addCacheNode(CacheNode(add_node_ip, add_node_port));
                }
                if (recv_data.hs_info().hashinfo_type() == HashSlotInfo::REMCACHE) {
                    string rem_node_ip = recv_data.hs_info().cache_node().ip();
                    int rem_node_port = recv_data.hs_info().cache_node().port();
                    std::lock_guard<std::mutex> lock(hashslot_mutex);
                    hashslot.remCacheNode(CacheNode(rem_node_ip, rem_node_port));
                }
                printf("the local hashslot has updated\n");

                // reply to master with HASHSLOTUPDATEACK
                ack_info.set_ack_type(AckInfo::HASHSLOTUPDATEACK);
                ack_info.set_ack_status(AckInfo::OK);
                send_data.set_data_type(CMCData::ACKINFO);
                send_data.mutable_ack_info()->CopyFrom(ack_info);
                bzero(send_buf_max, sizeof(send_buf_max));
                send_data.SerializeToArray(send_buf_max, sizeof(send_buf_max));
                int send_size = send(fd_array[3].fd, send_buf_max, send_data.ByteSizeLong(), 0);
                if (send_size < 0) {
                    std::cout << "Send HASHSLOTUPDATEACK failed!" << std::endl;
                }
            } else {
                printf("the type of recv_data is not CMCData::HASHSLOTINFO\n");
            }
        }

        /* -------------------- 发生本机标准输入事件 -------------------- */
        if (fd_array[0].revents & POLLIN) {
            std::string client_input_str;  // 客户端命令
            std::getline(std::cin, client_input_str);
            std::istringstream istr(client_input_str);
            string word;
            std::vector<std::string> word_v;
            while (istr >> word) {
                word_v.push_back(word);
            }
            std::string command, param_1, param_2;
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
                    std::cout << "too many arguments!" << std::endl;
                    continue;
                }
            }
            for (auto &c : command) {
                c = toupper(c);
            }
            std::cout << "Command: " << command << " " << param_1 << " " << param_2 << std::endl;

            /* ------------- 执行命令 ------------- */
            CMCData my_cmc_data;
            if (command == "QUIT" && param_count == 1) {
                std::cout << "The client has been close!" <<std::endl;
                break;  // break while(1), close(socketfd_to_master), return 0;
            } else if (command == "GET" && param_count == 2) {
                my_cmc_data = MakeCommandData(CommandInfo::GET, param_1, "");
            } else if (command == "SET" && param_count == 3) {
                my_cmc_data = MakeCommandData(CommandInfo::SET, param_1, param_2);
            } else if (command == "DEL" && param_count == 2) {
                my_cmc_data = MakeCommandData(CommandInfo::DEL, param_1, "");
            } else if (command == "TEST" && param_count == 1) {
                // sprawn a child process to do the test
                test_stop = false;
                std::thread test(startTest, 500, 1);       // 输入test开始循环测试，每次生成500个随机key，分别SET/GET，打印GET结果正确的次数，间隔1秒重复，Ctrl+\ 终止测试
                test.detach();
                continue;   // ignore the following 
            } else if (command == "TEST" && param_count == 2) {
                for (auto &c : param_1) {
                    c = toupper(c);
                }
                if (param_1 == "INF") {
                    test_stop = false;
                    std::thread test(startTest, 500, 0);       // 输入test inf开始循环测试，每次生成500个随机key，分别SET/GET，打印GET结果正确的次数，Ctrl+\ 终止测试
                    test.detach();
                    continue;   // ignore the following 
                }
            } else {
                std::cout << "Invalid Command!" << std::endl;
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(hashslot_mutex);
                strcpy(cache_ip, hashslot.getCacheAddr(param_1).first.c_str());  // 从hashslot中查询cache地址
                cache_port = hashslot.getCacheAddr(param_1).second;
            }
            std::cout << "will SendCommandData" << std::endl;
            CMCData result_data;                                                           // 访问的结果数据包
            if (SendCommandData(my_cmc_data, cache_ip, cache_port, result_data) == false)  // 发送命令数据包给cache
                std::cout << "sendCommandData fail." << std::endl;

            // 上面结果数据包result_data收到相关信息后，这里是需要以用户的视角把结果打印到屏幕的
            // 解析数据（主要是查看有没有想要查询的结果），并打印结果
            /* ------------- TODO ------------ */
            // 此处先把数据包信息打印出来
            string debug_str = result_data.DebugString();
            std::cout << debug_str << std::endl;
            std::cout << "DebugString() end!" << std::endl;
        }
    }
    close(socketfd_to_master);  // 关闭客户端套接字，客户端退出
    return 0;
}

void generateRandomKeyValuePairs(std::vector<std::pair<std::string, std::string>> &kv_vec)
{
    std::string k_temp, v_temp;
    std::srand(std::time(0));
    for (auto &elem : kv_vec) {
        for (int i = 0; i < KV_LEN; ++i) {
            switch (std::rand() % 3) {
                case 1:
                    k_temp.push_back('A' + std::rand() % 26);
                    v_temp.push_back('A' + std::rand() % 26);
                    break;
                case 2:
                    k_temp.push_back('a' + std::rand() % 26);
                    v_temp.push_back('a' + std::rand() % 26);
                    break;
                default:
                    k_temp.push_back('0' + std::rand() % 10);
                    v_temp.push_back('0' + std::rand() % 10);
                    break;
            }
        }
        elem = {k_temp, v_temp};
        k_temp.clear();
        v_temp.clear();
    }
}

// 间隔sleep_time秒，每次随机生成batch_size个key和value，先后进行设置和查询，打印查询正确的次数/总次数
// int sleep_time（秒）表示间隔停顿（便于观察Cache命中数量）
void startTest(int batch_size, int sleep_time)
{
    signal(SIGQUIT, stopTest);
    std::vector<std::pair<std::string, std::string>> kv_vec(batch_size);
    while (1) {
        generateRandomKeyValuePairs(kv_vec);
        CMCData send_cmc_data, recv_cmc_data;
        char cache_ip[INET_ADDRSTRLEN];
        uint32_t cache_port;
        int match_count = 0; 
        for (const auto &kv : kv_vec) {
            std::pair<std::string, int> cache_addr;
            {
                std::lock_guard<std::mutex> lock(hashslot_mutex);
                if (hashslot.numNodes() == 0) {
                    std::cout << "empty hashslot" << endl;
                    test_stop = true;
                    break;
                }
                cache_addr = hashslot.getCacheAddr(kv.first); std::cout << cache_addr.first << ":" << cache_addr.second;
            }
            bzero(cache_ip, INET_ADDRSTRLEN);
            strcpy(cache_ip, cache_addr.first.c_str());  // 从local_hashslot中查询cache地址
            cache_port = cache_addr.second;
            send_cmc_data = MakeCommandData(CommandInfo::SET, kv.first, kv.second);
            if (SendCommandData(send_cmc_data, cache_ip, cache_port, recv_cmc_data) == false) {
                std::cout << "SendCommandData failed." << std::endl;
            } else {
                std::cout << recv_cmc_data.ack_info().ack_status() << std::endl;
            }
            send_cmc_data.Clear();
            recv_cmc_data.Clear();
        }
        if (test_stop) {
            break;
        }
        for (const auto &kv : kv_vec) {
            std::pair<std::string, int> cache_addr;
            {
                std::lock_guard<std::mutex> lock(hashslot_mutex);
                if (hashslot.numNodes() == 0) {
                    std::cout << "empty hashslot" << endl;
                    test_stop = true;
                    break;
                }
                cache_addr = hashslot.getCacheAddr(kv.first);
            }
            bzero(cache_ip, INET_ADDRSTRLEN);
            strcpy(cache_ip, cache_addr.first.c_str());  // 从hashslot中查询cache地址
            cache_port = cache_addr.second;
            send_cmc_data = MakeCommandData(CommandInfo::GET, kv.first, "");
            if (SendCommandData(send_cmc_data, cache_ip, cache_port, recv_cmc_data) == false) {
                std::cout << "SendCommandData failed." << std::endl;
            } else {
                std::cout << recv_cmc_data.kv_data().value() << std::endl;
                if (recv_cmc_data.kv_data().value() == kv.second) {
                    ++match_count;
                }
            }
            send_cmc_data.Clear();
            recv_cmc_data.Clear();
        }
        std::cout << "Cache hit " << match_count << " out of " << batch_size << std::endl;
        if (test_stop) {
            break;
        }
        sleep(sleep_time);
    }
    return;
}
