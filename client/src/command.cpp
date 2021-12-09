#include "command.h"
const unsigned int MAX_BUF_SIZE = 212992;

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

/*----------------------------------- 发送命令数据 -----------------------------------*/
bool SendCommandData(const CMCData& cmc_data, const char* dst_ip, u_int16_t dst_port) {

    char send_buff[BUFSIZ];
    bzero(send_buff, BUFSIZ);

    // 打印待发送的CMCData信息
    string debug_str = cmc_data.DebugString();
    cout << debug_str << endl;

    int data_size = cmc_data.ByteSizeLong();
    cout << "data_size: " << data_size << endl;
    cmc_data.SerializeToArray(send_buff, data_size);
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

    // 阻塞连接master
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

    char recv_buf_max[MAX_BUF_SIZE];
    memset(recv_buf_max, 0, sizeof(recv_buf_max));
    int recv_size = recv(sockfd_to_cache, recv_buf_max, MAX_BUF_SIZE, 0);
    std::cout << "received: " << recv_size << " Bytes" << std::endl;

    if (recv_size <= 0) {
        throw std::runtime_error("recv() error \n");
    } else {
        // 解析回复数据包
        CMCData recv_cmc_data;
        recv_cmc_data.ParseFromArray(recv_buf_max, sizeof(recv_buf_max));

        // 此处先把数据包信息打印出来
        string recv_cmc_data_str = recv_cmc_data.DebugString();
        cout << recv_cmc_data_str << endl;
        cout << "DebugString() end!" << endl;

        close(sockfd_to_cache);  //当完成一次cache访问，就关闭与cache连接的套接字
        cout << "close(sockfd_to_cache);" << endl;
        cout << "--------------------------------------------------------" << endl;
        return true;
    }
}

// client主动向master拉取哈希槽信息的函数
// master_fd为传入参数：与master相连接的套接字
// slot为传入传出参数：返回返回后，HashSlot& slot被更新
bool get_slot(const int& master_fd, HashSlot& slot) {
    // 先准备好GETSLOT请求数据包，并序列化
    CMCData getslot_cmc_data = MakeCommandData(CommandInfo::GETSLOT, "", "");
    char send_buff[BUFSIZ];
    bzero(send_buff, BUFSIZ);
    int getslot_data_size = getslot_cmc_data.ByteSizeLong();
    // cout << "getslot_data_size " << getslot_data_size << endl;
    getslot_cmc_data.SerializeToArray(send_buff, getslot_data_size);
    // cout << "after SerializeToArray: " << strlen(send_buff) << endl;

    // 发送GETSLOT数据包
    int send_size = send(master_fd, send_buff, getslot_data_size, 0);
    cout << "client_send_size: " << send_size << endl;
    if (send_size < 0) {
        perror("send() error\n");
        return false;
    }

    // 发送后等待master返回HashSlotInfo，读取哈希槽信息
    char recv_buf_max[MAX_BUF_SIZE];
    memset(recv_buf_max, 0, sizeof(recv_buf_max));
    int recv_size = recv(master_fd, recv_buf_max, MAX_BUF_SIZE, 0);
    std::cout << "received: " << recv_size << " Bytes" << std::endl;
    if (recv_size <= 0) {
        perror("recv() error\n");
        return false;
    } else {
        // 更新本地hashslot
        CMCData slot_cmc_data;
        slot_cmc_data.ParseFromArray(recv_buf_max, sizeof(recv_size));
        slot.restoreFrom(slot_cmc_data.hs_info());
    }
    return true;
}