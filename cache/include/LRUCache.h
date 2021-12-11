#pragma once
#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <unordered_map>
#include <ctime>
#include <string>


// 用双向链表节点存储LRU机制下的key-value对，存在问题，key、value以及create_time的初始化，以及默认初始化对于不同Typekey、Typevalue还不能转换，time(NULL)精度存在问题，只能精确到秒，不适合高并发
// 数据结构Value保存key、value和创建时间
struct DListNode {
	std::string key;
	std::string value;
	DListNode* pre;
	DListNode* next;
	DListNode() : key(""), value(""), pre(nullptr), next(nullptr) {}
	DListNode(std::string _key, std::string _value) : key(_key), value(_value), pre(nullptr), next(nullptr) {}
};

// 
class LRUCache {
private:
	DListNode* head;
	DListNode* tail;
	int size;
	int capacity;
	void moveToHead(DListNode* node);
	void removeNode(DListNode* node);
	void addToHead(DListNode* node);
public:
	// LRU链表默认容量为10，并声明为explicit表明该构造函数是显式的，防止隐式转换
	explicit LRUCache(int capacity = 10);
	~LRUCache();
	std::string get(const std::string& key);
	bool set(const std::string& key, const std::string& value);
	bool deleteKey(std::string& key);
	std::unordered_map<std::string, DListNode*> m_map;		// 用哈希表存储缓存数据
};


#endif // !LRUCACHE_H
