#include "lru_replacer.hpp"

LRUReplacer::LRUReplacer(std::size_t capacity) : capacity(capacity) {}

int LRUReplacer::Evict() {
    if (lru_list.empty()) return -1;

    // El candidato a evictar es el ultimo (LRU)
    int frame_id = lru_list.back();
    lru_list.pop_back();
    lru_map.erase(frame_id);

    return frame_id;
}

void LRUReplacer::RecordAccess(int frame_id) {
    // Si ya existe en la lista, lo removemos primero para reinsertarlo al frente
    if (lru_map.count(frame_id)) {
        lru_list.erase(lru_map[frame_id]);
    }

    lru_list.push_front(frame_id);
    lru_map[frame_id] = lru_list.begin();
}

void LRUReplacer::Pin(int frame_id) {
    // El frame esta en uso, no puede ser evictado: lo sacamos del replacer
    if (lru_map.count(frame_id)) {
        lru_list.erase(lru_map[frame_id]);
        lru_map.erase(frame_id);
    }
}

void LRUReplacer::Unpin(int frame_id) {
    // pin_count llego a 0, el frame ya puede ser evictado: lo agregamos al frente
    if (!lru_map.count(frame_id)) {
        RecordAccess(frame_id);
    }
}

std::size_t LRUReplacer::Size() {
    return lru_list.size();
}