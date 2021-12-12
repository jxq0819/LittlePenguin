#ifndef MASTER_SERVER_H
#define MASTER_SERVER_H
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "TcpServer.h"
#include "ThreadPool.hpp"
#include "HashSlot.h"
#include "TaskQueue.h"

#define TASK_ADD 0
#define TASK_LOST 1
#define TASK_SHUT 2
#define DELAY_TIMEOUT 1       // 网络时延超时时间（秒）
#define HEARTBEAT_TIMEOUT 1   // 心跳超时时间（秒）
#define BLACKLIST_TIMEOUT 5   // 黑名单超时时间（秒）
#define BUFF_SIZE       2048        // 2KB
#define BUFF_SIZE_LONG  1024000     // 1MB

struct HeartbeatInfo {
    int32_t     time;
    int         status;
};

typedef std::pair<std::string, int> Addr;   // {ip, port} pair, used to identify each connection
struct addrHasher { 
    size_t operator()(const Addr &addr) const { 
        return std::hash<std::string>()(addr.first + "." + std::to_string(addr.second));
    }
};

class MasterServer : public TcpServer {
public:
    typedef int Op; // TASK_ADD/TASK_DEL

    explicit MasterServer(int maxWaiter = 5);
    void init();

    // 子类 纯虚函数实现
    void newConnection() override;
    void existConnection(int fd) override;

    int setnonblocking(int fd);

private:
    std::unique_ptr<ThreadPool> threadPool;  // 测试 
    std::unique_ptr<HashSlot> hash_slot_;
    std::unordered_set<Addr, addrHasher> links_;             // set of all connections' AddrPair
    std::unordered_set<Addr, addrHasher> client_links_;      // set of all clients' AddrPair
    std::unordered_map<Addr, HeartbeatInfo, addrHasher> cache_links_; // cache server Addrs mapped to their heartbeat info
    std::unordered_map<Addr, time_t, addrHasher> blacklist_; // blacklist, cache server Addr mapped to time_t，短时间内Master不会响应来自这些地址/端口的心跳包
    std::mutex hash_slot_mutex_;
    std::mutex links_mutex_;
    std::mutex client_links_mutex_;
    std::mutex cache_links_mutex_;
    std::mutex blacklist_mutex_;
    TaskQueue<std::pair<Op, Addr>> task_queue_;              // tasks to add/remove a cache server to/from [cache_links_]
    // callable objects to run in other threads
    static void startHeartbeartService(MasterServer*);
    static void startHashslotService(MasterServer*);
};

// utillity functions (will be in a seperate header)
bool checkAckInfo(int sockfd, int ackType);

#endif