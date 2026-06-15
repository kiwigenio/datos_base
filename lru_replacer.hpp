#ifndef LRU_REPLACER_HPP
#define LRU_REPLACER_HPP

#include <list>
#include <unordered_map>
#include <cstddef>


class LRUReplacer {
private:
    std::list<int> lru_list;

    std::unordered_map<int, std::list<int>::iterator> lru_map;

    std::size_t capacity;

public:
    LRUReplacer(std::size_t capacity);

    // Retorna el frame_id a evictar (el LRU), devuelve -1 si no hay candidatos
    int Evict();

    // Registra que un frame fue accedido (lo mueve al frente)
    void RecordAccess(int frame_id);

    // Saca un frame del replacer (cuando esta pinneado, no puede ser evictado)
    void Pin(int frame_id);

    // Devuelve un frame al replacer (cuando pin_count llega a 0)
    void Unpin(int frame_id);

    std::size_t Size();
};

#endif // LRU_REPLACER_HPP