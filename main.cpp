#include "storage_manager.hpp"
#include "page.hpp"
#include "buffer_pool_manager.hpp"
#include <cassert>

int main() {
    StorageManager sm("motor_db.bin");

    // Preparamos 5 páginas en disco para la prueba
    for (int i = 1; i <= 5; i++) {
        Page p(i);
        p.insertar_registro("pagina|" + std::to_string(i) + "|dato");
        sm.writePage(i, p);
    }
    std::cout << "=== 5 páginas escritas en disco ===" << std::endl;

    BufferPoolManager bpm(3, &sm);

    // --- FASE 1: llenamos el pool (3 frames, 3 páginas) ---
    std::cout << "\n--- FASE 1: Llenando el pool ---" << std::endl;

    Page* p1 = bpm.FetchPage(1);
    std::cout << "[Fetch p1] en RAM: " << p1->get_registro(0) << std::endl;
    bpm.UnpinPage(1, false);  // pin_count = 0, entra al LRU

    Page* p2 = bpm.FetchPage(2);
    std::cout << "[Fetch p2] en RAM: " << p2->get_registro(0) << std::endl;
    bpm.UnpinPage(2, false);  // pin_count = 0, entra al LRU

    Page* p3 = bpm.FetchPage(3);
    std::cout << "[Fetch p3] en RAM: " << p3->get_registro(0) << std::endl;
    bpm.UnpinPage(3, false);  // pin_count = 0, entra al LRU

    // LRU actual (frente=MRU, final=LRU): [3, 2, 1]

    // --- FASE 2: accedemos a p1 para que deje de ser LRU ---
    std::cout << "\n--- FASE 2: Re-accediendo a página 1 (sube en LRU) ---" << std::endl;

    Page* p1_again = bpm.FetchPage(1);
    std::cout << "[Fetch p1 again] en RAM: " << p1_again->get_registro(0) << std::endl;
    bpm.UnpinPage(1, false);

    // LRU actual: [1, 3, 2]  → la página 2 es ahora la LRU

    // --- FASE 3: forzamos eviction pidiendo página 4 ---
    std::cout << "\n--- FASE 3: Fetch página 4 (pool lleno, debe evictar página 2) ---" << std::endl;

    Page* p4 = bpm.FetchPage(4);
    std::cout << "[Fetch p4] en RAM: " << p4->get_registro(0) << std::endl;
    bpm.UnpinPage(4, false);

    // LRU actual: [4, 1, 3]  → página 2 fue evictada

    // --- FASE 4: modificamos página 3 y verificamos dirty + flush ---
    std::cout << "\n--- FASE 4: Modificando página 3 (dirty flag) ---" << std::endl;

    Page* p3_mod = bpm.FetchPage(3);
    p3_mod->insertar_registro("registro|nuevo|modificado");
    std::cout << "[Fetch p3] registro nuevo: " << p3_mod->get_registro(1) << std::endl;
    bpm.UnpinPage(3, true);  // marcamos dirty

    std::cout << "[FlushPage p3] ";
    if (bpm.FlushPage(3)) {
        std::cout << "página 3 sucia escrita al disco correctamente." << std::endl;
    }

    // --- FASE 5: forzamos eviction de página dirty (página 3) ---
    std::cout << "\n--- FASE 5: Forzando eviction de página dirty (página 3) ---" << std::endl;

    // Llenamos con p5 y p2 para forzar que p3 sea evictada
    Page* p5 = bpm.FetchPage(5);
    std::cout << "[Fetch p5] en RAM: " << p5->get_registro(0) << std::endl;
    bpm.UnpinPage(5, false);

    Page* p2_again = bpm.FetchPage(2);
    std::cout << "[Fetch p2 again] en RAM: " << p2_again->get_registro(0) << std::endl;
    bpm.UnpinPage(2, false);

    // p3 es ahora LRU y será evictada automáticamente en el siguiente fetch
    Page* p1_final = bpm.FetchPage(1);
    std::cout << "[Fetch p1 final] LRU evictó p3 automáticamente, p1 en RAM: " 
              << p1_final->get_registro(0) << std::endl;
    bpm.UnpinPage(1, false);

    return 0;
}