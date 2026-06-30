#pragma once 
#include "b_plus_tree_page.hpp"
#include "rid.hpp"
#include <utility>

#define LEAf_PAGE_HEADER_SIZE 24 

#define LEAF_PAGE_SIZE ((4096 - LEAf_PAGE_HEADER_SIZE)/ sizeof(MappingType))


template <typename KeyType>
class BPlusTreeLeafPage : public BPlusTreePage { 
    private: 
        using MappingType = std::pair<KeyType, RID>; 

        int32_t next_page_id_;

        MappingType array_[0];
    public: 
        void Init (int32_t page_id, int32_t parent_id = -1, int max_size = LEAF_PAGE_SIZE){ 
            SetPageType(IndexPageType::LEAF_PAGE); 
            SetPageId(page_id); 
            SetParentPageId(parent_id);
            SetSize(0);
            SetMaxSize(max_size);
            next_page_id_ = -1; 
        }       

        int32_t GetNextPageId() const { return next_page_id_;}
        void SetNextPageId(int32_t next_page_id) { next_page_id_ =next_page_id;}

        KeyType KeyAt(int index) const { return array_[index].first;}
        RID ValueAt(int index) const { return array_[index].second;}

        bool Lookup(const KeyType &key, RID *value) const { 
            for( int i =0; i < GetSize(); ++i) { 
                if( array_[i].first == key){ 
                    *value  =  array_[i].second;
                    return true; 
                }
            }
            return false;
        }

        int Insert(const KeyType &key, const RID &value) {
            int target_index = 0;
            int current_size = GetSize();

            // 1. Buscamos la posición correcta para mantener el orden
            while (target_index < current_size && array_[target_index].first < key) {
                target_index++;
            }

            // 2. Movemos todos los elementos mayores un espacio hacia la derecha
            for (int i = current_size; i > target_index; --i) {
                array_[i] = array_[i - 1];
            }
            array_[target_index].first = key;
            array_[target_index].second = value;
            
            IncreaseSize(1);
            return GetSize();
        }
        void MoveHalfTo(BPlusTreeLeafPage *recipient) {
            int total_size = GetSize();
            int half_index = total_size / 2;
            int move_count = total_size - half_index;

            // 1. Copiamos la segunda mitad al nuevo nodo
            for (int i = 0; i < move_count; ++i) {
                recipient->array_[i] = array_[half_index + i];
            }

            // 2. Actualizamos los tamaños (ahora ambos tienen la mitad)
            recipient->SetSize(move_count);
            this->SetSize(half_index);

           
            recipient->SetNextPageId(this->GetNextPageId());
            this->SetNextPageId(recipient->GetPageId());
        }
        
        void MoveAllTo(BPlusTreeLeafPage *recipient) {
            int start_idx = recipient->GetSize();
            for (int i = 0; i < GetSize(); ++i) {
                recipient->array_[start_idx + i] = array_[i];
            }
            recipient->SetNextPageId(GetNextPageId()); // Actualizamos la lista enlazada
            recipient->IncreaseSize(GetSize());
            SetSize(0); // Esta página se queda vacía lista para morir
        }

        // Para el PRÉSTAMO (Izquierda a Derecha): Mueve el ÚLTIMO elemento al FRENTE del vecino
        void MoveLastToFrontOf(BPlusTreeLeafPage *recipient) {
            // Hacemos espacio en el vecino moviendo todo a la derecha
            for (int i = recipient->GetSize(); i > 0; --i) {
                recipient->array_[i] = recipient->array_[i - 1];
            }
            // Le pasamos nuestro último elemento
            recipient->array_[0] = array_[GetSize() - 1];
            recipient->IncreaseSize(1);
            IncreaseSize(-1);
        }

        // Para el PRÉSTAMO (Derecha a Izquierda): Mueve el PRIMER elemento al FINAL del vecino
        void MoveFirstToEndOf(BPlusTreeLeafPage *recipient) {
            // Ponemos nuestro primer elemento al final del vecino
            recipient->array_[recipient->GetSize()] = array_[0];
            recipient->IncreaseSize(1);
            
            // Tapamos el hueco en nuestra hoja moviendo todo a la izquierda
            for (int i = 0; i < GetSize() - 1; ++i) {
                array_[i] = array_[i + 1];
            }
            IncreaseSize(-1);
        }
        int RemoveAndDeleteRecord(const KeyType &key) {
            int target_index = -1;
            for (int i = 0; i < GetSize(); ++i) {
                if (array_[i].first == key) {
                    target_index = i;
                    break;
                }
            }
            if (target_index == -1) return GetSize(); 

            for (int i = target_index; i < GetSize() - 1; ++i) {
                array_[i] = array_[i + 1];
            }
            IncreaseSize(-1);
            return GetSize();
        }

};