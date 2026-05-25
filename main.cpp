#include "storage_manager.hpp"
#include "page.hpp"
#include "buffer_pool_manager.hpp"

int main() {
    StorageManager sm("motor_db.bin");

    Page page1(1);
    int slot_prueba1 = page1.insertar_registro("kerin|larico|22");
    int slot_prueba2 = page1.insertar_registro("santiesteban|gomez|30");
    int slot_prueba3 = page1.insertar_registro("maria|lopez|25");

    if ( sm.writePage(page1.header.page_id, page1)) {
        cout << "Página escrita exitosamente." << endl;
    } else {
        cout << "Error al escribir la página." << endl;
    }

     Page page_lectura;

     if(sm.readPage(1, page_lectura)){
        cout << " prueba 1 "<< slot_prueba1 <<" : " << page_lectura.get_registro(slot_prueba1) <<endl;
        cout << " prueba 2 "<< slot_prueba2 <<" : " << page_lectura.get_registro(slot_prueba2) <<endl;
        cout << " prueba 3 "<< slot_prueba3 <<" : " << page_lectura.get_registro(slot_prueba3) <<endl;
    }

    cout << "--- PRUEBA BUFFER POOL MANAGER ---" << endl;
    {
        BufferPoolManager bpm(3, &sm); 

        cout << "\n[A] Fetch de la Página 1 (Leyendo del disco a la RAM)..." << endl;
        Page* p1_en_ram = bpm.FetchPage(1);
        
        if (p1_en_ram != nullptr) {
            cout << "-> Éxito: La página 1 está en el Buffer Pool." << endl;
            cout << "-> Datos desde RAM: " << p1_en_ram->get_registro(slot_prueba1) << endl;
            
            bpm.UnpinPage(1, false); 
        }

        cout << "\n[B] Fetch de la Página 2 (Nueva) y escritura en RAM..." << endl;
        Page* p2_en_ram = bpm.FetchPage(2);
        
        if (p2_en_ram != nullptr) {
            int nuevo_slot = p2_en_ram->insertar_registro("buffer|pool|99");
            cout << "-> Éxito: Registro insertado en la página 2 en el Buffer Pool." << endl;
            cout << "-> Leyendo registro insertado: " << p2_en_ram->get_registro(nuevo_slot) << endl;
            
            bpm.UnpinPage(2, true); 
        }

        cout << "\n[C] Forzando guardado de RAM al Disco (FlushPage)..." << endl;
        if (bpm.FlushPage(2)) {
            cout << "-> Éxito: La página 2 sucia fue escrita permanentemente en 'motor_db.bin'." << endl;
        } else {
            cout << "-> Error al hacer Flush de la página 2." << endl;
        }
    }
    return 0;

}