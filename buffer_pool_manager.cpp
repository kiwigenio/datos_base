#include "buffer_pool_manager.hpp"

BufferPoolManager::BufferPoolManager(size_t size, StorageManager* disk_manager) 
    : pool_size(size), disk_manager(disk_manager), next_page_id_(0) {
    
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

Page* BufferPoolManager::NewPage(int32_t* page_id){
    if (free_list.empty()) {
        std::cerr << "[BufferPool] ERROR: RAM llena, no se puede crear página nueva." << std::endl;
        return nullptr;
    }

    // 2. Obtenemos un frame libre
    int frame_id = free_list.front();
    free_list.pop_front();

    // 3. Asignamos un nuevo ID de página y avanzamos el contador
    *page_id = next_page_id_++;

    // 4. Inicializamos el frame y la página en memoria
    pool[frame_id].Reset(*page_id);

    // 5. Registramos en la tabla de páginas
    page_table[*page_id] = frame_id;
    pool[frame_id].pin_count = 1;  // La fijamos (pin) porque quien la pidió la va a usar
    
    // Al ser nueva, la marcamos como dirty para obligar a que se guarde en disco eventualmente
    pool[frame_id].is_dirty = true; 

    std::cout << "[BufferPool] Nueva página creada con ID: " << *page_id << std::endl;

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

bool BufferPoolManager::DeletePage(int32_t page_id){
    if (page_table.find(page_id) == page_table.end()) {
        return true; 
    }

    int frame_id = page_table[page_id];

    // Si alguien la está usando, no podemos borrarla
    if (pool[frame_id].pin_count > 0) {
        std::cerr << "[BufferPool] ERROR: No se puede borrar página " << page_id << " porque está en uso." << std::endl;
        return false;
    }
    page_table.erase(page_id);
    pool[frame_id].Reset(0);
    free_list.push_back(frame_id);
    return true;
}