#include "buffer_pool_manager.hpp"

BufferPoolManager::BufferPoolManager(size_t size, StorageManager* disk_manager) 
    : pool_size(size), disk_manager(disk_manager) {
    
    pool.resize(pool_size); 
    for (size_t i = 0; i < pool_size; ++i) {
        free_list.push_back(i); 
    }
}

BufferPoolManager::~BufferPoolManager() {
    for (auto const& [page_id, frame_id] : page_table) {
        FlushPage(page_id);
    }
}

Page* BufferPoolManager::FetchPage(int32_t page_id) {
    if (page_table.find(page_id) != page_table.end()) {
        int frame_id = page_table[page_id];
        pool[frame_id].pin_count++;
        return &pool[frame_id].page;
    }

    if (free_list.empty()) {
        std::cerr << "[BufferPool] ERROR: RAM llena y no hay algoritmo de reemplazo." << std::endl;
        return nullptr; 
    }

    int frame_id = free_list.front();
    free_list.pop_front();

    pool[frame_id].Reset(page_id);
    if (!disk_manager->readPage(page_id, pool[frame_id].page)) {
        std::cout << "[BufferPool] Página " << page_id << " no existe en disco, se crea nueva." << std::endl;
    }

    page_table[page_id] = frame_id;
    pool[frame_id].pin_count = 1;
    pool[frame_id].is_dirty = false;

    return &pool[frame_id].page;
}

bool BufferPoolManager::UnpinPage(int32_t page_id, bool is_dirty) {
    if (page_table.find(page_id) == page_table.end()) {
        return false; 
    }

    int frame_id = page_table[page_id];
    
    if (pool[frame_id].pin_count <= 0) return false;

    pool[frame_id].pin_count--;

    if (is_dirty) {
        pool[frame_id].is_dirty = true;
    }

    return true;
}

bool BufferPoolManager::FlushPage(int32_t page_id) {
    if (page_table.find(page_id) == page_table.end()) {
        return false; 
    }

    int frame_id = page_table[page_id];

    if (pool[frame_id].is_dirty) {
        if (disk_manager->writePage(page_id, pool[frame_id].page)) {
            pool[frame_id].is_dirty = false; 
            return true;
        }
        return false;
    }
    return true; 
}