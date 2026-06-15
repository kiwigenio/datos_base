#include "b_plus_tree.hpp"

template<typename KeyType>

Page* BPlusTree<KeyType>::FindLeafPage(const KeyType &key){ 
    if (IsEmpty()){
        return nullptr;
    }
    Page* curr_page  = bpm_-> FetchPage(root_page_id_);
    auto* curr_node = reinterpret_cast<BPlusTreePage*>(curr_page->data);

    while(!curr_node->IsLeafPage()){
        auto* internal_node = reinterpret_cast<BPlusTreeInternalPage<KeyType>*>(curr_node);

        int32_t child_page_id = internal_node->Lookup(key);

        bpm_->UnpinPage(curr_node->GetPageId(), false); // No modificamos el nodo, así que is_dirty = false

        curr_page = bpm_->FetchPage(child_page_id);
        curr_node = reinterpret_cast<BPlusTreePage*>(curr_page->data);
    }
    return curr_page;
};

template <typename KeyType>
bool BPlusTree<KeyType>::GetValue(const KeyType &key, RID *result) {
    // 1. Encontramos la hoja donde debería estar el dato
    Page* leaf_page = FindLeafPage(key);
    if (leaf_page == nullptr) {
        return false; // El árbol está vacío
    }

    // 2. Casteamos la página a un Nodo Hoja
    auto* leaf_node = reinterpret_cast<BPlusTreeLeafPage<KeyType>*>(leaf_page->data);

    // 3. Buscamos la clave exacta dentro de la hoja (usando el método que hicimos en la semana 9)
    bool found = leaf_node->Lookup(key, result);

    // 4. Liberamos la hoja (nunca olvides el Unpin)
    bpm_->UnpinPage(leaf_node->GetPageId(), false);

    return found;
}


template <typename KeyType>
bool BPlusTree<KeyType>::Insert(const KeyType &key, const RID &value) {
    // CASO 1: EL ÁRBOL ESTÁ VACÍO
    if (IsEmpty()) {
        int32_t new_page_id;
        // Pedimos una página nueva para que sea la raíz
        Page* root_page = bpm_->NewPage(&new_page_id);
        
        if (root_page != nullptr) {
            root_page_id_ = new_page_id;
            
            // Le ponemos el molde de hoja (porque al inicio la raíz es una hoja)
            auto* root_node = reinterpret_cast<BPlusTreeLeafPage<KeyType>*>(root_page->data);
            root_node->Init(root_page_id_, -1,3);
            
            // Insertamos el primer dato
            root_node->Insert(key, value);
            
            // Liberamos y guardamos (dirty = true porque la modificamos)
            bpm_->UnpinPage(root_page_id_, true);
            return true;
        }
        return false;
    }

    // 1. Buscamos la hoja correcta donde debería ir esta llave
    Page* leaf_page = FindLeafPage(key);
    if (leaf_page == nullptr) return false;

    auto* leaf_node = reinterpret_cast<BPlusTreeLeafPage<KeyType>*>(leaf_page->data);

    // 2. Comprobamos que la llave no exista ya (si no permites duplicados)
    RID temp_rid;
    if (leaf_node->Lookup(key, &temp_rid)) {
        // La llave ya existe, no hacemos nada
        bpm_->UnpinPage(leaf_node->GetPageId(), false);
        return false; 
    }

    // 3. Insertamos en orden
    leaf_node->Insert(key, value);

    if (leaf_node->GetSize() >= leaf_node->GetMaxSize()) {
        std::cout << "[ALERTA] La hoja " << leaf_node->GetPageId() 
                  << " se lleno. Necesitamos hacer SPLIT!" << std::endl;
    
        if (leaf_node->GetSize() >= leaf_node->GetMaxSize()) {
            std::cout << "\n[SPLIT] La hoja " << leaf_node->GetPageId() << " se lleno. Dividiendo..." << std::endl;
            
            // a) Pedimos una nueva página para la nueva hoja
            int32_t new_leaf_page_id;
            Page* new_leaf_page = bpm_->NewPage(&new_leaf_page_id);
            auto* new_leaf_node = reinterpret_cast<BPlusTreeLeafPage<KeyType>*>(new_leaf_page->data);
            
            // Inicializamos la nueva hoja
            new_leaf_node->Init(new_leaf_page_id, leaf_node->GetParentPageId(), leaf_node->GetMaxSize());
            
            // b) Pasamos la mitad de los datos
            leaf_node->MoveHalfTo(new_leaf_node);
            
            // c) La clave que "sube" al padre es la primera clave de la nueva hoja derecha
            KeyType split_key = new_leaf_node->KeyAt(0);

            // d) Si la hoja que se dividió era la raíz, necesitamos crear una NUEVA RAÍZ INTERNA
            if (leaf_node->IsRootPage()) {
                int32_t new_root_page_id;
                Page* new_root_page = bpm_->NewPage(&new_root_page_id);
                auto* new_root_node = reinterpret_cast<BPlusTreeInternalPage<KeyType>*>(new_root_page->data);
                
                
                new_root_node->Init(new_root_page_id, -1);
                
                new_root_node->SetValueAt(0, leaf_node->GetPageId()); // Hijo Izquierdo
                new_root_node->SetKeyAt(1, split_key);                // Clave semáforo
                new_root_node->SetValueAt(1, new_leaf_page_id);       // Hijo Derecho
                new_root_node->SetSize(2); // Tiene 2 punteros ocupados

                leaf_node->SetParentPageId(new_root_page_id);
                new_leaf_node->SetParentPageId(new_root_page_id);


                root_page_id_ = new_root_page_id;

                bpm_->UnpinPage(new_root_page_id, true);
                std::cout << "[SPLIT EXITOSO] Nueva raiz interna creada con ID: " << new_root_page_id << std::endl;
            }
            else {
                // Si NO es la raíz, tenemos que avisarle al PADRE que tiene un nuevo hijo
                int32_t parent_id = leaf_node->GetParentPageId();
                
                // Pedimos la página del padre al Buffer Pool
                Page* parent_page = bpm_->FetchPage(parent_id);
                auto* parent_node = reinterpret_cast<BPlusTreeInternalPage<KeyType>*>(parent_page->data);

                // Le insertamos la nueva llave semáforo y la página del nuevo hermano
                parent_node->InsertNodeAfter(split_key, new_leaf_page_id);

               
                bpm_->UnpinPage(parent_id, true);
                
                std::cout << "[AVISO AL PADRE] El nodo interno " << parent_id 
                        << " ha recibido a la nueva hoja " << new_leaf_page_id << std::endl;
            }

            bpm_->UnpinPage(new_leaf_page_id, true);
        }
    }

    // 5. Liberamos la página
    bpm_->UnpinPage(leaf_node->GetPageId(), true); // dirty = true
    return true;
}
template class BPlusTree<int>;