#ifndef CACHE_NODE_H
#define CACHE_NODE_H

#include <string>
#include <bitset>

#define HASHSLOT_SIZE 16384

class CacheNode {
    friend class HashSlot;
public:
    CacheNode(std::string ip, int p): ip_(ip), port_(p) { name_ = ip_ + std::to_string(port_); }
    // CacheNode(std::string n, std::string ip, int p): name_(n), ip_(ip), port_(p) { }
    std::string name() const { return name_; }
    std::string ip() const { return ip_; }
    int port() const { return port_; }
    int numslots() const { return slots_.count(); }
private:
    std::string name_;
    std::string ip_;
    int port_;
    std::bitset<HASHSLOT_SIZE> slots_;
};

#endif

