#pragma once

#include <cmcdata.pb.h>

#include "LRUCache.h"
#include "TcpServer.h"
#include "ThreadPool.hpp"


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
    // inline bool getCacheStatus() {return m_cache_status;};

  private:
    std::unique_ptr<ThreadPool> threadPool;  // 测试用，暂时8个线程
    LRUCache m_cache;                        // CacheServer类内LRU表
    bool m_cache_status;                     // cache状态默认是良好的，cache发生错误的时候，会把这个状态设置成false

    // 执行相应的命令
    bool executeCommand(const CommandInfo& cmd_info, CMCData& response_data);
};
