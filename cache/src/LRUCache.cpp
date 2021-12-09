#include "LRUCache.h"

LRUCache::LRUCache(int capacity) {
    this->capacity = capacity;
    this->size = 0;
    head = new DListNode();
    tail = new DListNode();
    head->next = tail;
    tail->pre = head;
}

LRUCache::~LRUCache() {
    if (head != nullptr) {
        delete head;
        head = nullptr;
    }
    if (tail != nullptr) {
        delete tail;
        tail = nullptr;
    }
    for (auto& a : m_map) {
        if (a.second != nullptr) {
            delete a.second;
            a.second = nullptr;
        }
    }
}

void LRUCache::moveToHead(DListNode* node) {
    removeNode(node);
    addToHead(node);
}

void LRUCache::removeNode(DListNode* node) {
    node->pre->next = node->next;
    node->next->pre = node->pre;
}

void LRUCache::addToHead(DListNode* node) {
    node->pre = head;
    node->next = head->next;
    head->next->pre = node;
    head->next = node;
}

std::string LRUCache::get(const std::string& key) {
    if (key.empty() && !m_map.count(key)) {
        return "";
    }
    DListNode* node = m_map[key];
    moveToHead(node);
    return node->value;
}

bool LRUCache::set(const std::string& key, const std::string& value) {
    if (key.empty()) return false;

    if (!m_map.count(key)) {
        DListNode* node = new DListNode(key, value);
        m_map[key] = node;
        addToHead(node);
        ++size;
        if (size > capacity) {  // 对于由于cache server存储上限时，利用LRU淘汰机制淘汰对应的key-value对
            DListNode* tmp = tail->pre;
            removeNode(tmp);
            m_map.erase(tmp->key);
            delete tmp;
            --size;
        }
    } else {
        m_map[key]->value = value;
        moveToHead(m_map[key]);
    }
    return true;
}

bool LRUCache::deleteKey(std::string& key) {
	if (key.empty()) return false;
	
    if (m_map.count(key)) {
        DListNode* node = m_map[key];
        m_map[key] = node;
        removeNode(node);
        m_map.erase(node->key);
        delete node;
        --size;
        return true;
    } else
        return false;
}
