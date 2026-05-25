#include "storage_manager.hpp"
#include "page.hpp"

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
    return 0;

}