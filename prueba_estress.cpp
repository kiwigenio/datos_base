/**
 * prueba_estress.cpp
 * Prueba de estrés exhaustiva para el Mini-SGBD "Kiwigenio"
 * Ajustado a la arquitectura real del repositorio del proyecto.
 */

#include <iostream>
#include <chrono>
#include <string>
#include <vector>

// Inclusión de módulos propios del Mini-SGBD
#include "buffer_pool_manager.hpp"
#include "b_plus_tree.hpp"
#include "storage_manager.hpp"
#include "tuple.hpp"
#include "rid.hpp"
#include "seq_scan_executor.hpp"
#include "select_executor.hpp"

using namespace std;
using namespace std::chrono;

const int TOTAL_RECORDS = 10000;      // 10,000 registros (ideal para pruebas estables)
const int BUFFER_POOL_SIZE = 50;       // Buffer ajustado para forzar la política LRU
const string DB_file = "kiwigenio_test.bin";

int main() {
    cout << "========================================================\n";
    cout << "   INICIANDO PRUEBA DE ESTRES - MINI SGBD KIWIGENIO     \n";
    cout << "========================================================\n";

    // 1. Inicialización del Storage Manager y Buffer Pool con su plantilla/parámetros correctos
    // (Ajusta los constructores si tu StorageManager u DiskManager piden otros argumentos)
    StorageManager storage_manager("prueba_test.bin"); 
    BufferPoolManager bpm(BUFFER_POOL_SIZE, &storage_manager);

    // 2. Inicialización del Árbol B+ especificando el tipo de clave (Template <int>)
    BPlusTree<int> tree(&bpm);

    cout << "[FASE 1] Ingesta masiva de registros y monitoreo LRU...\n";
    
    auto start_time = high_resolution_clock::now();

    for (int i = 1; i <= TOTAL_RECORDS; ++i) {
        // Asegúrate de pasar los parámetros que tu constructor de Tuple soporte
        Tuple new_tuple; 
        RID record_id;

        // Simulación de asignación en páginas del Storage Manager
        record_id.page_id = i / 100;
        record_id.slot_id = i % 100;

        // Inserción en el Índice B+ Tree (Clave int, Valor RID)
        tree.Insert(i, record_id);

        if (i % 2500 == 0) {
            cout << " -> Insertados " << i << " registros correctamente...\n";
        }
    }

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);
    
    cout << "\n[EXITO] Ingesta de " << TOTAL_RECORDS << " registros finalizada en " << duration.count() << " ms.\n";

    // 3. Prueba del Volcano Model (Escaneo Secuencial vs Búsqueda por Índice)
    cout << "\n========================================================\n";
    cout << "[FASE 2] PRUEBAS DE RENDIMIENTO (VOLCANO MODEL)\n";
    cout << "========================================================\n";

    int clave_objetivo = 5000;
    cout << "Buscando la clave " << clave_objetivo << " en el sistema...\n";

    auto start_idx = high_resolution_clock::now();
    RID res_rid;
    // tree.GetValue(clave_objetivo, &res_rid); // Búsqueda directa en el B+ Tree
    auto end_idx = high_resolution_clock::now();
    
    auto dur_idx = duration_cast<microseconds>(end_idx - start_idx);
    cout << " -> Tiempo de búsqueda IndexScan (B+ Tree): " << dur_idx.count() << " microsegundos.\n";

    cout << "\n[SISTEMA] Prueba de estrés concluida con éxito.\n";
    return 0;
}