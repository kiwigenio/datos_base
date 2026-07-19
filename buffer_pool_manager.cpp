#include "buffer_pool_manager.hpp"
#include <iomanip>

BufferPoolManager::BufferPoolManager(size_t size, StorageManager* disk_manager) 
    : pool_size(size), disk_manager(disk_manager), next_page_id_(0) , replacer(size) {
    
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
        hits_++;
        int frame_id = page_table[page_id];
        pool[frame_id].pin_count++;
        replacer.Pin(frame_id); 
        return &pool[frame_id].page;

    }
    misses_++;

    int frame_id = -1;

    if (!free_list.empty()) {
        // Todavía hay frames sin usar
        frame_id = free_list.front();
        free_list.pop_front();
    } else {
        // Pool lleno: pedimos al LRU que evicte un frame
        frame_id = replacer.Evict();
        if (frame_id == -1) {
            std::cerr << "[BufferPool] ERROR: todos los frames estan pinneados, no se puede evictar." << std::endl;
            return nullptr;
        }
        for (auto const& [pid, fid] : page_table) {
            if (fid == frame_id) {
                if (pool[frame_id].is_dirty) {
                    disk_manager->writePage(pid, pool[frame_id].page);
                }
                page_table.erase(pid);
                break;
            }
        }
    }

    pool[frame_id].Reset(page_id);
    if (!disk_manager->readPage(page_id, pool[frame_id].page)) {
        std::cout << "[BufferPool] Página " << page_id << " no existe en disco, se crea nueva." << std::endl;
    }

    page_table[page_id] = frame_id;
    pool[frame_id].pin_count = 1;
    pool[frame_id].is_dirty = false;
    replacer.Pin(frame_id);

    return &pool[frame_id].page;
}

Page* BufferPoolManager::NewPage(int32_t* page_id){
    int frame_id = -1;

    if (!free_list.empty()) {
        frame_id = free_list.front();
        free_list.pop_front();
    } else {
        // igual que FetchPage: pedimos al LRU que evicte
        frame_id = replacer.Evict();
        if (frame_id == -1) {
            std::cerr << "[BufferPool] ERROR: todos los frames están pinneados." << std::endl;
            return nullptr;
        }
        // limpiamos la page_table del frame evictado
        for (auto const& [pid, fid] : page_table) {
            if (fid == frame_id) {
                if (pool[frame_id].is_dirty) {
                    disk_manager->writePage(pid, pool[frame_id].page);
                }
                page_table.erase(pid);
                break;
            }
        }
    }

    *page_id = next_page_id_++;
    pool[frame_id].Reset(*page_id);
    page_table[*page_id] = frame_id;
    pool[frame_id].pin_count = 1;
    pool[frame_id].is_dirty = true;
    replacer.Pin(frame_id);

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

    if (pool[frame_id].pin_count == 0) {
        replacer.Unpin(frame_id);
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

void BufferPoolManager::MostrarEstado() {
    std::cout << "\n=== ESTADO DEL BUFFER POOL (MEMORIA RAM) ===" << std::endl;
    std::cout << "Capacidad: " << pool_size << " frames | Libres: " << free_list.size() << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    
    for (size_t i = 0; i < pool_size; ++i) {
        std::cout << "[Frame " << i << "] ";
        
        // Verificamos si este frame está en la lista de vacíos
        bool esta_libre = false;
        for (size_t libre : free_list) {
            if (libre == i) { esta_libre = true; break; }
        }
        
        if (esta_libre) {
            std::cout << "--- VACIO ---" << std::endl;
        } else {
            // Mostramos los metadatos
            std::cout << "Page_ID: " << pool[i].page.header.page_id 
                      << " | Pin_Count: " << pool[i].pin_count 
                      << " | Dirty: " << (pool[i].is_dirty ? "SI" : "NO") << std::endl;
            
            // Mostramos el contenido de la página
            int slots = pool[i].page.header.num_slots;
            if (slots > 0) {
                for (int j = 0; j < slots; j++) {
                    std::string texto = pool[i].page.get_registro(j);
                    if (texto != "registro borrado" && texto != "Slot Id inválido") {
                        std::cout << "            -> Slot " << j << ": '" << texto << "'" << std::endl;
                    }
                }
            } else {
                std::cout << "            -> (Pagina sin registros)" << std::endl;
            }
        }
    }
    std::cout << "============================================" << std::endl;
}

void BufferPoolManager::ReportHitRate() const {
    uint64_t total = hits_ + misses_;
    std::cout << "\n=== ESTADISTICAS DEL BUFFER POOL ===" << std::endl;
    std::cout << "Hits   (RAM):  " << hits_ << std::endl;
    std::cout << "Misses (Disco): " << misses_ << std::endl;
    std::cout << "Total de accesos: " << total << std::endl;
    if (total > 0) {
        double hit_rate = (double)hits_ / total * 100.0;
        std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) << hit_rate << "%" << std::endl;
    } else {
        std::cout << "Hit Rate: N/A (sin accesos)" << std::endl;
    }
    std::cout << "=====================================" << std::endl;
}

void BufferPoolManager::ResetStats() {
    hits_ = 0;
    misses_ = 0;
    std::cout << "[Stats] Contadores reiniciados." << std::endl;
}