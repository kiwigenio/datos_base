#pragma once 
#include "b_plus_tree_page.hpp"
#include <utility>


#define INTERNAL_PAGE_HEADER_SIZE 20

#define INTERNAL_PAGE_SIZE ((4096 -INTERNAL_PAGE_HEADER_SIZE) / sizeof(MappingType))

template<typename KeyType> 
class BPlusTreeInternalPage : public BPlusTreePage{
    private: 
        using MappingType = std::pair<KeyType, int32_t>;

        MappingType array_[0];
    public:
        void Init(int32_t page_id, int32_t parent_id, int max_size) {
            SetPageId(page_id);
            SetParentPageId(parent_id);
            SetMaxSize(max_size);
            SetSize(0);

            // BARRIDO DE MEMORIA BASURA:
            // Limpiamos todo el arreglo de punteros para que no queden IDs fantasma como 66145.
            // Asumiendo que tu arreglo interno se llama array_
            for (int i = 0; i <= max_size; ++i) {
                array_[i].second = -1; // -1 representa INVALID_PAGE_ID
            }
        }

        KeyType KeyAt(int index) const{ 
            return array_[index].first; 
        }

        void SetKeyAt(int index, const KeyType& key ) { 
            array_[index].first = key;
        }

        int32_t ValueAt(int index) const { 
            return array_[index].second;
        }

        void SetValueAt(int index, int32_t value) { 
            array_[index].second = value;  
        }

        int32_t Lookup(const KeyType &key ) const { 
            for( int i = 1; i < GetSize(); ++i){ 
                if ( key< array_[i].first){ 
                    return array_[i-1].second;
                }
            }
            return array_[GetSize() -1].second;
        }
        void InsertNodeAfter(const KeyType &new_key, int32_t new_page_id) {
            int current_size = GetSize();
            int target_index = 1; 

            // Buscamos dónde debe ir la nueva clave
            while (target_index < current_size && array_[target_index].first < new_key) {
                target_index++;
            }

            // Movemos a la derecha todos los elementos mayores
            for (int i = current_size; i > target_index; --i) {
                array_[i] = array_[i - 1];
            }

            // Insertamos
            array_[target_index].first = new_key;
            array_[target_index].second = new_page_id;
            
            IncreaseSize(1);
        }     
        void Remove(int index) {
        for (int i = index; i < GetSize() - 1; ++i) {
            array_[i] = array_[i + 1];
        }
        IncreaseSize(-1);
        }
        int ValueIndex(const int &value) const {
        for (int i = 0; i < GetSize(); ++i) {
            if (array_[i].second == value) return i;
        }
        return -1;
        }   
};