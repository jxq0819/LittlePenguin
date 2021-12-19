#ifndef HASH_SLOT_H
#define HASH_SLOT_H

#include <iostream>
#include <list>

#include "CacheNode.h"
// Protobuf message headers
#include "cmcdata.pb.h"

class HashSlot {
    friend std::ostream& operator<<(std::ostream&, const HashSlot&);
public:
    // Operations
    HashSlot() = default;
    HashSlot(const HashSlot&);              // copy constructor
    HashSlot(const HashSlotInfo&);          // build the HashSlot from a HashSlotInfo

    HashSlot& operator=(const HashSlot&);   // assignment operator
    void addCacheNode(const CacheNode&);    // add a cache node
    void remCacheNode(const CacheNode&);    // remove a cache node
    const CacheNode &cback() { return node_list_.back(); }                          // returns a reference to const to the last inserted cache node
    std::list<CacheNode>::const_iterator cbegin() { return node_list_.cbegin(); }   // returns a const_iterator to the first cache node
    std::list<CacheNode>::const_iterator cend() { return node_list_.cend(); }       // returns a const_iterator to the one-past-the-last cache node
    int numNodes() { return num_nodes_; }   // returns the number of nodes in use

    void saveTo(HashSlotInfo&) const;       // save the HashSlot to a HashSlotInfo
    void restoreFrom(const HashSlotInfo&);  // restore hashslot from a HashSlotInfo

    void printCacheNode(std::ostream& os, int i) const { os << slots_[i]->name(); } 
    std::pair<std::string, int> getCacheAddr(const std::string key);
private:
    int num_nodes_ = 0;                     // num of nodes in use
    std::list<CacheNode> node_list_;        // list of all nodes in use
    CacheNode *slots_[HASHSLOT_SIZE];       // pointer to the Hash Slots
};

// Interface
std::ostream& operator<<(std::ostream&, const HashSlot&);   // print the node status

#endif
