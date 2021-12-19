#ifndef CACHE_NODE_H
#define CACHE_NODE_H

#include <string>
#include <bitset>

#define HASHSLOT_SIZE 16384     // Maximum number of cache nodes in the system

class CacheNode {
    friend class HashSlot;
public:
    CacheNode(std::string ip, int p): ip_(ip), port_(p) { name_ = ip_ + "." + std::to_string(port_); }    // Usage: CacheNode("127.0.0.1", 5005);
    std::string name() const { return name_; }          // returns the name of the cache node
    std::string ip() const { return ip_; }              // returns the ip of the cache node
    int port() const { return port_; }                  // returns the port of the cache node
    int numslots() const { return slots_.count(); }     // returns the number of slots this cache node covers
    constexpr bool operator[]( std::size_t pos ) const { return slots_[pos]; }                      // 返回pos处的哈希槽值0/1，Usage: cache_node[pos];
    std::bitset<HASHSLOT_SIZE>::reference operator[]( std::size_t pos ) { return slots_[pos]; }     // 返回pos处哈希槽比特位的reference，Usage：cache_node[pos] = true/false;
private:
    std::string name_;
    std::string ip_;
    int port_;
    std::bitset<HASHSLOT_SIZE> slots_;
};

#endif

