#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "TcpServer.h"
#include "ThreadPool.hpp"
#include "HashSlot.h"
#include "TaskQueue.h"

struct HeartbeatInfo {
    int32_t     time;
    int         status;
};

#define TASK_ADD 0
#define TASK_REM 1

typedef std::pair<std::string, int> AddrPair;   // {ip, port} pair, used to identify each connection
struct addrHasher { 
    size_t operator()(const AddrPair &addr) const { 
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
    // Static hash function for type Addr
    // static size_t addrHasher(const AddrPair &addr) {
    //     std::cout << addr.first + "." + std::to_string(addr.second) << std::endl;
    //     return std::hash<std::string>()(addr.first + "." + std::to_string(addr.second));
    // }

private:
    std::unique_ptr<ThreadPool> threadPool;  // 测试 
    std::unique_ptr<HashSlot> hash_slot_;
    std::unordered_set<AddrPair, addrHasher> links_;             // Set of all connections' Addr
    std::unordered_set<AddrPair, addrHasher> client_links_;      // Set of all clients' Addr
    std::unordered_map<AddrPair, HeartbeatInfo, addrHasher> cache_links_; // Cache server Addrs mapped to their heartbeat info
    TaskQueue<std::pair<Op, AddrPair>> task_queue_;              // Tasks to add/remove a cache server to/from [cache_links_]
    // callable objects to run in other threads
    static void startHeartbeartService(MasterServer*);
    static void startHashslotService(MasterServer*);
};

// Utillity functions (Should be in a seperate header ?)
bool checkAckInfo(int sockfd, int ackType);