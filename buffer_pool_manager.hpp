#ifndef BUFFER_POOL_MANAGER_HPP
#define BUFFER_POOL_MANAGER_HPP

#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>
#include "frame.hpp"
#include "storage_manager.hpp"
#include "lru_replacer.hpp"

class BufferPoolManager {
private:
    size_t pool_size;
    std::vector<Frame> pool;
    std::unordered_map<int32_t, int> page_table;
    std::list<int> free_list;
    StorageManager* disk_manager;
    LRUReplacer replacer;

    int32_t next_page_id_;

    uint64_t hits_ = 0;
    uint64_t misses_ = 0;

public:
    BufferPoolManager(size_t size, StorageManager* disk_manager);
    ~BufferPoolManager();

    // API Principal
    Page* FetchPage(int32_t page_id);
    Page* NewPage(int32_t* page_id);
    bool DeletePage(int32_t page_id);
    bool UnpinPage(int32_t page_id, bool is_dirty);
    bool FlushPage(int32_t page_id);

    void MostrarEstado();

    void ReportHitRate() const;
    void ResetStats();
    int32_t GetNextPageId() const { return next_page_id_; }
    void SetNextPageId(int32_t next_id) { next_page_id_ = next_id; }
};

#endif // BUFFER_POOL_MANAGER_HPP