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

template <typename KeyType>
void BPlusTree<KeyType>::Remove(const KeyType &key) {
    if (IsEmpty()) return;

    // Buscamos la hoja donde debería estar la clave
    Page* leaf_page = FindLeafPage(key);
    if (leaf_page == nullptr) return;

    auto* leaf_node = reinterpret_cast<BPlusTreeLeafPage<KeyType>*>(leaf_page->data);

    int old_size = leaf_node->GetSize();
    // Borramos el registro usando la función que creaste en la Fase 1
    int new_size = leaf_node->RemoveAndDeleteRecord(key);

    bool is_dirty = (old_size != new_size);

    // ¡EL NÚCLEO DEL BONUS! Si borramos algo y el nodo cae en Underflow:
    if (is_dirty && new_size < leaf_node->GetMinSize()) {
        CoalesceOrRedistribute(leaf_node);
    }

    // Liberamos la página de la RAM
    bpm_->UnpinPage(leaf_node->GetPageId(), is_dirty);
}

// AJUSTAR LA RAÍZ (Cuando el árbol pierde altura)
template <typename KeyType>
bool BPlusTree<KeyType>::AdjustRoot(BPlusTreePage *old_root_node) {
    // Si la raíz era una hoja y se quedó vacía, el árbol muere.
    if (old_root_node->IsLeafPage() && old_root_node->GetSize() == 0) {
        root_page_id_ = -1;
        bpm_->DeletePage(old_root_node->GetPageId());
        return true;
    }
    
    // Si la raíz interna se quedó con un solo hijo, ese hijo sube a ser la nueva raíz.
    if (!old_root_node->IsLeafPage() && old_root_node->GetSize() == 1) {
        auto *internal_root = reinterpret_cast<BPlusTreeInternalPage<KeyType> *>(old_root_node);
        root_page_id_ = internal_root->ValueAt(0); 
        
        Page *new_root_page = bpm_->FetchPage(root_page_id_);
        auto *new_root_node = reinterpret_cast<BPlusTreePage *>(new_root_page->data);
        new_root_node->SetParentPageId(-1);
        
        bpm_->UnpinPage(root_page_id_, true);
        bpm_->DeletePage(old_root_node->GetPageId());
        return true;
    }
    return false;
}

// EL GERENTE DE EMERGENCIAS (Decide si prestar o fusionar)
template <typename KeyType>
template <typename N>
bool BPlusTree<KeyType>::CoalesceOrRedistribute(N *node) {
    if (node->IsRootPage()) {
        return AdjustRoot(node);
    }
    if (node->GetSize() >= node->GetMinSize()) {
        return false; 
    }

    // Buscamos al Padre para encontrar a los hermanos
    Page *parent_page = bpm_->FetchPage(node->GetParentPageId());
    auto *parent = reinterpret_cast<BPlusTreeInternalPage<KeyType> *>(parent_page->data);
    
    int my_index = parent->ValueIndex(node->GetPageId());
    
    // Elegimos al hermano izquierdo si es posible, si no, al derecho
    int sibling_index = (my_index == 0) ? 1 : my_index - 1;
    Page *sibling_page = bpm_->FetchPage(parent->ValueAt(sibling_index));
    N *sibling = reinterpret_cast<N *>(sibling_page->data);

    bool parent_dirty = false;

    if (sibling->GetSize() > sibling->GetMinSize()) {
        // Borrowing
        Redistribute(sibling, node, my_index);
        bpm_->UnpinPage(sibling->GetPageId(), true); 
        parent_dirty = true;                         
    } else {
        // Merging)
        parent_dirty = Coalesce(&sibling, &node, &parent, my_index);
        bpm_->UnpinPage(sibling->GetPageId(), true);
    }

    bpm_->UnpinPage(parent->GetPageId(), parent_dirty);
    return true; 
}

template <typename KeyType>
template <typename N>
void BPlusTree<KeyType>::Redistribute(N *neighbor_node, N *node, int index) {
    // Para simplificar la complejidad de esta versión, implementamos el préstamo a nivel de hojas
    if (node->IsLeafPage()) {
        auto *leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType> *>(node);
        auto *neighbor = reinterpret_cast<BPlusTreeLeafPage<KeyType> *>(neighbor_node);
        
        // Traemos al padre para actualizar su semáforo
        Page *parent_page = bpm_->FetchPage(leaf->GetParentPageId());
        auto *parent = reinterpret_cast<BPlusTreeInternalPage<KeyType> *>(parent_page->data);

        if (index == 0) {
            // Nosotros somos el hijo [0], así que el vecino es el hermano DERECHO.
            // Le robamos su PRIMER elemento y lo ponemos al FINAL nuestro.
            neighbor->MoveFirstToEndOf(leaf);
            // Actualizamos el semáforo del hermano derecho en el padre
            parent->SetKeyAt(1, neighbor->KeyAt(0)); 
        } else {
            // Nosotros somos un hijo > 0, así que el vecino es el hermano IZQUIERDO.
            // Le robamos su ÚLTIMO elemento y lo ponemos al FRENTE nuestro.
            neighbor->MoveLastToFrontOf(leaf);
            // Actualizamos nuestro propio semáforo en el padre
            parent->SetKeyAt(index, leaf->KeyAt(0)); 
        }
        
        bpm_->UnpinPage(parent->GetPageId(), true);
    }
}

// Coalesce / Merging
template <typename KeyType>
template <typename N>
bool BPlusTree<KeyType>::Coalesce(N **neighbor_node, N **node, BPlusTreeInternalPage<KeyType> **parent, int index) {
    if ((*node)->IsLeafPage()) {
        auto *leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType> *>(*node);
        auto *neighbor = reinterpret_cast<BPlusTreeLeafPage<KeyType> *>(*neighbor_node);

        // Para fusionar siempre de Derecha a Izquierda, ordenamos los punteros
        if (index == 0) {
            // Si somos el hijo [0], el neighbor es el derecho. Intercambiamos los roles
            // para que 'neighbor' sea siempre el que se queda vivo (el izquierdo).
            std::swap(leaf, neighbor);
            std::swap(node, neighbor_node);
            index = 1; // Actualizamos el índice 
        }

        // Movemos TODOS los datos de la hoja derecha hacia la hoja izquierda
        leaf->MoveAllTo(neighbor);
        
        bpm_->DeletePage(leaf->GetPageId()); 

        (*parent)->Remove(index);
        
        return (*parent)->GetSize() < (*parent)->GetMinSize();
    }
    return false;
}

template <typename KeyType>
void BPlusTree<KeyType>::PrintTree() {
    if (IsEmpty()) {
        std::cout << "  [El arbol B+ esta vacio]" << std::endl;
        return;
    }

    std::queue<int> q;
    q.push(root_page_id_);

    while (!q.empty()) {
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            int curr_page_id = q.front();
            q.pop();
            
            Page *page = bpm_->FetchPage(curr_page_id);
            auto *node = reinterpret_cast<BPlusTreePage *>(page->data);
            
            if (node->IsLeafPage()) {
                auto *leaf = reinterpret_cast<BPlusTreeLeafPage<KeyType> *>(node);
                std::cout << "[HOJA " << curr_page_id << ": ";
                for (int j = 0; j < leaf->GetSize(); j++) {
                    std::cout << leaf->KeyAt(j) << " ";
                }
                std::cout << "]  ";
            } else {
                auto *internal = reinterpret_cast<BPlusTreeInternalPage<KeyType> *>(node);
                std::cout << "[INTERNO " << curr_page_id << ": ";
                // En nodos internos, la primera clave (índice 0) está vacía, empezamos en 1
                for (int j = 1; j < internal->GetSize(); j++) {
                    std::cout << internal->KeyAt(j) << " ";
                }
                std::cout << "]  ";
                
                // Encolamos a todos los hijos para el siguiente nivel
                for (int j = 0; j < internal->GetSize(); j++) {
                    q.push(internal->ValueAt(j));
                }
            }
            bpm_->UnpinPage(curr_page_id, false);
        }
        std::cout << std::endl; // Salto de línea por cada nivel del árbol
    }
}

template class BPlusTree<int>;