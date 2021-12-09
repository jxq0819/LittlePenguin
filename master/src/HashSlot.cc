#include "HashSlot.h"

#include <iostream>
#include <algorithm>
#include <sstream>

#include <cassert>

HashSlot::HashSlot(const HashSlot& hs): num_nodes_(hs.num_nodes_), node_list_(hs.node_list_)
{                                                       // Copy initialise the node list
    for (auto &node : node_list_) {                     // Initialise HashSlot.slots_
        int cur = node.slots_._Find_first();
        for (int i = 0; i < node.numslots(); ++i, cur = node.slots_._Find_next(cur)) {
            slots_[cur] = &node;
        }
    }
}

HashSlot::HashSlot(const HashSlotInfo &hash_slot_info): num_nodes_(hash_slot_info.cache_nodes_size())
{
    for (int i = 0; i < num_nodes_; ++i) {
        node_list_.push_back(CacheNode(hash_slot_info.cache_nodes(i).name(),
                                        hash_slot_info.cache_nodes(i).ip(),
                                        hash_slot_info.cache_nodes(i).port()));
        const std::string &str = hash_slot_info.cache_nodes(i).slots();
        for (int j = 0; j < HASHSLOT_SIZE/8; ++j) {
            char temp = str[j];
            for (int k = 0; k < 8; ++k) {
                node_list_.back().slots_[8*j + k] = temp & (1 << (7-k));
            }
        }
    }
    // Then reconstruct the CacheNode *slots_[HASHSLOT_SIZE]
    for (auto &node : node_list_) {
        int cur = node.slots_._Find_first();
        for (int i = 0; i < node.numslots(); ++i, cur = node.slots_._Find_next(cur)) {
            slots_[cur] = &node;
        }
    }
}

HashSlot& HashSlot::operator=(const HashSlot &hs) 
{
    num_nodes_ = hs.num_nodes_;
    node_list_ = hs.node_list_;
    // Then reconstruct the CacheNode *slots_[HASHSLOT_SIZE]
    for (auto &node : node_list_) {
        int cur = node.slots_._Find_first();
        for (int i = 0; i < node.numslots(); ++i, cur = node.slots_._Find_next(cur)) {
            slots_[cur] = &node;
        }
    }
    return *this;
}

void HashSlot::addCacheNode(const CacheNode &node)
{
    // Check if the node already exists (by name)
    auto node_it = std::find_if(node_list_.begin(), node_list_.end(), [&node](const CacheNode &n){ return n.name() == node.name(); });
    assert(node_it == node_list_.end());            // 添加列表中已存在的服务器时报错
    // Add the new node to the node list
    node_list_.push_back(node);
    ++num_nodes_;
    // Reshard
    if (num_nodes_ == 1) {
        node_list_.back().slots_.set();    // Set all bits to 1
        std::fill(std::begin(slots_), std::end(slots_), &node_list_.back());
        return;
    }
    // Calculate the slot count for each node
    int num_slots_per_node = HASHSLOT_SIZE / num_nodes_, residue = HASHSLOT_SIZE % num_nodes_;
    // For node 1 to num_nodes-1, remove slots
    int node_index = 0;
    for (auto it = node_list_.begin(); node_index < num_nodes_ - 1; ++it, ++node_index) {
        int change_count = node_index < residue ? it->numslots() - (num_slots_per_node + 1) : it->numslots() - num_slots_per_node;
        assert(change_count >= 0);                  // 16384个Node后再添加服务器时报错
        for (int i = 0; i < change_count; ++i) {
            auto cur = it->slots_._Find_first();    // Position of the 1(slot) to be modified
            it->slots_.reset(cur);
            node_list_.back().slots_.set(cur);
            slots_[cur] = &node_list_.back();
        }
    }
}

void HashSlot::remCacheNode(const CacheNode &node)
{
    // Find the node to be removed (by name)
    auto node_it = std::find_if(node_list_.begin(), node_list_.end(), [&node](const CacheNode &n){ return n.name() == node.name(); });
    assert(node_it != node_list_.end());            // 删除列表中不存在的服务器时报错
    CacheNode temp = *node_it;                      // Copy initialise a temporary
    node_list_.erase(node_it);
    --num_nodes_;
    // Reshard
    if (num_nodes_ == 0) {
        std::fill(std::begin(slots_), std::end(slots_), nullptr);
        return;
    }
    // Calculate the slot count for each node
    int num_slots_per_node = HASHSLOT_SIZE / num_nodes_, residue = HASHSLOT_SIZE % num_nodes_;
    // For all remaining nodes, add slots
    int node_index = 0;
    for (auto it = node_list_.begin(); node_index < num_nodes_; ++it, ++node_index) {
        int change_count = node_index < residue ? (num_slots_per_node + 1) - it->numslots() : num_slots_per_node - it->numslots();
        // assert(change_count >= 0);
        for (int i = 0; i < change_count; ++i) {
            auto cur = temp.slots_._Find_first();    // Position of the 1(slot) to be modified
            temp.slots_.reset(cur);
            it->slots_.set(cur);
            slots_[cur] = &*it;
        }
    }
}

void HashSlot::saveTo(HashSlotInfo &hash_slot_info) const
{
    for (auto &node : node_list_) {
        auto node_info = hash_slot_info.add_cache_nodes();
        node_info->set_name(node.name());
        node_info->set_ip(node.ip());
        node_info->set_port(node.port());
        // set the slots
        auto slots_ptr = node_info->mutable_slots();
        for (int i = 0; i < HASHSLOT_SIZE/8; ++i) {
            char temp = 0;
            for (int j = 0; j < 8; ++j) {
                temp += node.slots_[8*i + j] << (7-j);      // Start from the left-most bit
            }
            slots_ptr->push_back(temp);
        }
    }
}

void HashSlot::restoreFrom(const HashSlotInfo &hash_slot_info)
{
    num_nodes_ = hash_slot_info.cache_nodes_size();
    node_list_.clear();                                     // Empty the current HashSlot
    for (int i = 0; i < num_nodes_; ++i) {
        node_list_.push_back(CacheNode(hash_slot_info.cache_nodes(i).name(),
                                        hash_slot_info.cache_nodes(i).ip(),
                                        hash_slot_info.cache_nodes(i).port()));
        const std::string &str = hash_slot_info.cache_nodes(i).slots();
        for (int j = 0; j < HASHSLOT_SIZE/8; ++j) {
            char temp = str[j];
            for (int k = 0; k < 8; ++k) {
                node_list_.back().slots_[8*j + k] = temp & (1 << (7-k));
            }
        }
    }
    // Then reconstruct the CacheNode *slots_[HASHSLOT_SIZE]
    for (auto &node : node_list_) {
        int cur = node.slots_._Find_first();
        for (int i = 0; i < node.numslots(); ++i, cur = node.slots_._Find_next(cur)) {
            slots_[cur] = &node;
        }
    }
}

// Interface
std::ostream& operator<<(std::ostream &os, const HashSlot &hs)
{
    os << "Num of Cache Servers: " << hs.num_nodes_ << std::endl;
    for (const auto &node : hs.node_list_) {
        os << "Server " << node.name() << ", " << node.ip() << ":" << node.port() << ", " 
           << "slot count:" << node.numslots() << std::endl;
    }
    return os;
}
