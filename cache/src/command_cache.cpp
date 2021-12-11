/* command_cache这个文件是从client端代码中复用了MakeCommandData、SendCommandData函数 */

#include "command_cache.h"

/*----------------------------------- 生成命令数据 -----------------------------------*/
CMCData MakeCommandData(const ::CommandInfo_CmdType cmd_type, const string& param1, const string& param2) {
    // 新建CommandInfo命令类
    CommandInfo cmd_info;
    // 新建数据类（发送给目的主机的数据）
    CMCData cmc_data;
    // 给CommandInfo命令信息对象赋值
    cmd_info.set_cmd_type(cmd_type);  // cmd_type may be GET/SET/...
    if (!param1.empty()) {
        cmd_info.set_param1(param1);
        if (!param2.empty()) {
            cmd_info.set_param2(param2);
        }
    }
    // 给待发送CMCData数据对象赋值
    cmc_data.set_data_type(CMCData::COMMANDINFO);
    auto cmd_info_ptr = cmc_data.mutable_cmd_info();
    cmd_info_ptr->CopyFrom(cmd_info);
    return cmc_data;
}

/*------ 发送命令数据包send_data，并等待对端回复，传出收到的数据包recv_data ------*/
bool SendCommandData(const CMCData& send_data, const char* dst_ip, u_int16_t dst_port, CMCData& recv_data) {
    char send_buff[BUFSIZ];
    bzero(send_buff, BUFSIZ);

    // 打印待发送的CMCData信息
    string debug_str = send_data.DebugString();
    cout << debug_str << endl;

    int data_size = send_data.ByteSizeLong();
    cout << "data_size: " << data_size << endl;
    send_data.SerializeToArray(send_buff, data_size);
    cout << "after SerializeToArray: " << strlen(send_buff) << endl;

    if (dst_ip == "" || dst_port == 0) {
        cout << "ip or port is null" << endl;
        return false;
    }
    if (dst_port <= 1024 || dst_port >= 65535) {
        cout << "invalid port." << endl;
        return false;
    }
    struct sockaddr_in cache_addr;
    bzero(&cache_addr, 0);
    cache_addr.sin_family = AF_INET;
    cache_addr.sin_port = htons(dst_port);
    if (inet_pton(AF_INET, dst_ip, &cache_addr.sin_addr.s_addr) < 0) {
        perror("inet_pton() error\n");
        return false;
    }

    // 创建与cache_server对应的套接字文件描述符
    int sockfd_to_cache = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_to_cache < 0) {
        perror("socket() error\n");
        return false;
    }

    // 阻塞连接cache
    if (connect(sockfd_to_cache, (struct sockaddr*)&cache_addr, sizeof(sockaddr_in)) < 0) {
        perror("connect() error\n");
        close(sockfd_to_cache);
        return false;
    }
    cout << "connect cache server success!" << endl;

    // 向cache端发送数据
    int send_size = send(sockfd_to_cache, send_buff, data_size, 0);
    cout << "client_send_size: " << send_size << endl;
    if (send_size < 0) {
        std::cout << "GET Command sending failed!" << std::endl;
        return false;
    }

    // 读取对端的回复消息

    char recv_buf_max[BUFSIZ];
    memset(recv_buf_max, 0, sizeof(recv_buf_max));
    int recv_size = recv(sockfd_to_cache, recv_buf_max, BUFSIZ, 0);
    std::cout << "received: " << recv_size << " Bytes" << std::endl;

    if (recv_size <= 0) {
        throw std::runtime_error("recv() error \n");
    } else {
        // 传出回复数据包，写入信息
        recv_data.ParseFromArray(recv_buf_max, sizeof(recv_buf_max));

        // 关闭此次查询TCP连接服务
        close(sockfd_to_cache);  //当完成一次cache访问，就关闭与cache连接的套接字
        cout << "close(sockfd_to_cache);" << endl;
        cout << "--------------------------------------------------------" << endl;
        return true;
    }
}
