#ifndef BUFFER_POOL_MANAGER_HPP
#define BUFFER_POOL_MANAGER_HPP

#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>
#include "frame.hpp"
#include "storage_manager.hpp"

class BufferPoolManager {
private:
    size_t pool_size;
    std::vector<Frame> pool;
    std::unordered_map<int32_t, int> page_table;
    std::list<int> free_list;
    StorageManager* disk_manager;

    int32_t next_page_id_;

public:
    BufferPoolManager(size_t size, StorageManager* disk_manager);
    ~BufferPoolManager();

    // API Principal
    Page* FetchPage(int32_t page_id);
    Page* NewPage(int32_t* page_id);
    bool DeletePage(int32_t page_id);
    bool UnpinPage(int32_t page_id, bool is_dirty);
    bool FlushPage(int32_t page_id);
};

#endif // BUFFER_POOL_MANAGER_HPP