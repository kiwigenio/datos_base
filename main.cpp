#include "storage_manager.hpp"
#include "page.hpp"

int main() {
    cout<< "Prueba uno , creando el storage manager con todo por defecto "<<endl;
    {
        StorageManager sm("motor_db.bin");
        sm.inicializar_archivos();

    }
    cout<< "prueba dos , se creo el archivo con 100 paginas vacias "<<endl;

    return 0;

}