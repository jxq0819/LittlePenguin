#ifndef HASH_SLOT_H
#define HASH_SLOT_H

#include <iostream>
#include <list>

#include "CacheNode.h"
// Protobuf message headers
#include "cmcdata.pb.h"
#include "crc16.h"

class HashSlot {
    friend std::ostream& operator<<(std::ostream&, const HashSlot&);

  public:
    // Operations
    HashSlot() = default;
    HashSlot(const HashSlot&);      // copy constructor - 节点变化时可能需要
    HashSlot(const HashSlotInfo&);  // Build the HashSlot from a HashSlotInfo

    HashSlot& operator=(const HashSlot&);  // assignment operator
    void addCacheNode(const CacheNode&);
    void remCacheNode(const CacheNode&);

    void saveTo(HashSlotInfo&) const;       // Save the HashSlot to a HashSlotInfo
    void restoreFrom(const HashSlotInfo&);  // Restore from a HashSlotInfo

    void printCacheNode(std::ostream& os, int i) const { os << slots_[i]->name(); }
    std::pair<std::string, int> getCacheAddr(const std::string key);

  private:
    int num_nodes_ = 0;                // Num of cluster nodes
    std::list<CacheNode> node_list_;   // List of all nodes in use
    CacheNode* slots_[HASHSLOT_SIZE];  // Pointer to the Hash Slots
};

// Interface
std::ostream& operator<<(std::ostream&, const HashSlot&);  // print node status

#endif
