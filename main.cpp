#include <iostream>
#include "storage_manager.hpp"
#include "buffer_pool_manager.hpp"
#include "b_plus_tree.hpp"
// Nota: BPlusTree.hpp ya debe incluir a los demás.

using namespace std;

int main() {
    cout << "=== TEST SEMANA 10: INSERCION Y BUSQUEDA ===" << endl;

    StorageManager* disk_manager = new StorageManager("test_db.bin");
    disk_manager->inicializar_archivos(); 
    BufferPoolManager* bpm = new BufferPoolManager(10, disk_manager);

    // 1. Creamos nuestro Arbol B+
    BPlusTree<int> tree(bpm);

    // 2. Probamos insertar datos
    cout << "Insertando ID 10..." << endl; tree.Insert(10, RID(1, 1));
    cout << "Insertando ID 5..." << endl;  tree.Insert(5, RID(2, 2));
    cout << "Insertando ID 20..." << endl; tree.Insert(20, RID(3, 3));
    
    // ¡AQUÍ EXPLOTA Y HACE SPLIT!
    cout << "Insertando ID 15 (Provoca Split)..." << endl; 
    tree.Insert(15, RID(4, 4));

    // Si el árbol sobrevivió y el enrutador funciona, debería encontrar el 20 bajando por la nueva raíz
    RID result_2;
    cout << "\nBuscando ID 20 despues del Split: ";
    if (tree.GetValue(20, &result_2)) {
        cout << "Encontrado en Pagina " << result_2.page_id << endl;
    }

    // 3. Probamos buscar los datos
    RID result;
    cout << "\nBuscando ID 5: ";
    if (tree.GetValue(5, &result)) {
        cout << "Encontrado en Pagina " << result.page_id << ", Slot " << result.slot_id << endl;
    } else {
        cout << "No encontrado." << endl;
    }

    cout << "Buscando ID 20: ";
    if (tree.GetValue(20, &result)) {
        cout << "Encontrado en Pagina " << result.page_id << ", Slot " << result.slot_id << endl;
    } else {
        cout << "No encontrado." << endl;
    }

    cout << "Buscando ID 99 (No existe): ";
    if (tree.GetValue(99, &result)) {
        cout << "Encontrado!" << endl;
    } else {
        cout << "No encontrado. (Correcto)" << endl;
    }

    delete bpm;
    delete disk_manager;

    return 0;
}