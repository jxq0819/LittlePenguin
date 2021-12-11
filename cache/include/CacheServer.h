#pragma once

#include <cmcdata.pb.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zconf.h>

#include <ctime>
#include <iostream>
#include <string>

#include "HashSlot.h"
#include "LRUCache.h"
#include "TcpServer.h"
#include "ThreadPool.hpp"
#include "cmcdata.pb.h"
#include "command_cache.h"

using namespace std;

class CacheServer : public TcpServer {
  public:
    // CacheServer构造函数，默认为20个
    explicit CacheServer(int maxWaiter = 20);

    // 子类 纯虚函数实现
    void newConnection() override;
    void existConnection(int fd) override;
    // 解析CMC数据包
    bool parseData(const CMCData& recv_data, CMCData& response_data);
    // 开始心跳线程
    bool beginHeartbeatThread(const struct sockaddr_in& master_addr);
    // inline void setCacheStatus(bool&& s) {m_cache_status = s;};
    // inline bool getCacheSatus() {return m_cache_status;};

  private:
    std::unique_ptr<ThreadPool> threadPool;  // 测试用，暂时8个线程
    LRUCache m_cache;                        // CacheServer类内LRU表

    std::mutex m_epoll_ctl_mutex;     // 用于多线程操作epoll_ctl函数的互斥锁
    std::mutex m_cache_mutex;         // 用于多线程访问m_cache时加的互斥锁
    std::mutex m_hashslot_mutex;      // 用于多线程访问旧的hashslot时加的互斥锁
    std::mutex m_hashslot_new_mutex;  // 用于多线程访问新的hashslot时加的互斥锁

    HashSlot m_hashslot;      // CacheServer本地哈希槽（也就是我们说的old哈希槽，不进行数据迁移的时候和新的是一样的）
    HashSlot m_hashslot_new;  // CacheServer本地最新哈希槽，当完成数据迁移后需要赋值给m_hashslot
    bool m_is_migrating;      // cache是否在数据迁移状态，默认没有正在进行数据迁移
    // 当在数据迁移的时候设置为true，此时假如客户端查询（客户端肯定查过它本地的hashslot，但这个hashslot是旧版本的）
    // 所以这个时候需要：1、先查询本地LRU缓存，

    bool m_cache_status;  // cache状态默认是良好的，cache发生错误的时候，会把这个状态设置成false

    // 执行相应的命令
    bool executeCommand(const CommandInfo& cmd_info, CMCData& response_data);

    // 数据迁移处理函数
    bool dataMigration(const HashSlotInfo& hs_info, CMCData& response_data);
};
