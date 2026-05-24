#include "storage_manager.hpp"
#include "page.hpp"
#include <cassert>

int main() {
    StorageManager sm("motor_db.bin");
    cout<< "Prueba uno , creando el storage manager con todo por defecto "<<endl;
    {
        sm.inicializar_archivos();

    }
    cout<< "paso la prueba, se creo el archivo con 100 paginas vacias";

    cout<<" prueba numero 2 , insertando registors de longitud variable en cascada"<<endl;
    int id_pagina_asignada = -1; 
    int slot_juan = -1; 
    int slot_maria = -1; 
    {
        id_pagina_asignada = sm.encontrar_pagina_con_espacio(7);
        assert(id_pagina_asignada != -1); // Aseguramos que se encontró una página con espacio suficiente

        Page pagina; 
        bool read_success = sm.readPage(id_pagina_asignada, pagina);
        assert(read_success==true);

        slot_juan = pagina.insertar_registro("Juan|20");
        assert(slot_juan == 0);
        
        slot_maria = pagina.insertar_registro("Maria|Gomez|Trabajadora_Social|45");
        assert(slot_maria == 1);
        
        bool write_success = sm.writePage(id_pagina_asignada, pagina);
        assert(write_success==true);
        uint16_t espacio_libre = pagina.header.espacio_libre_hacia_arriba - pagina.header.espacio_libre_hacia_abajo;
        sm.actualizar_espacio(id_pagina_asignada, espacio_libre);
        sm.guardar_mapa();
    }
    cout<<"paso la prueba de insertar regsitros de longitud variable y guardarlo en el disco duro "<<endl;

    cout<< "prueba numero 3 , verificar persistencia de los datos ";
    {
        Page pagina_recuperada; 
        bool read_success = sm.readPage(id_pagina_asignada, pagina_recuperada); 
        assert(read_success==true);
        string registro_juan = pagina_recuperada.get_registro(slot_juan);
        string registro_maria = pagina_recuperada.get_registro(slot_maria);
        assert(registro_juan == "Juan|20");
        assert(registro_maria == "Maria|Gomez|Trabajadora_Social|45");

    }
    cout << "paso la prueba de persistencia de datos, se recuperaron los datos anteriores";
    
    cout <<"prueba numero 4 , eliminar un registro y probar el reciclaje de ranuras"<<endl;
    {
        Page pagina; 
        sm.readPage(id_pagina_asignada, pagina);
        bool delete_success = pagina.borrar_registro(slot_juan);
        assert(delete_success==true);
        assert(pagina.get_registro(slot_juan)== "registro borrado");

        int nuevo_slot = pagina.insertar_registro("Pedro|30");
        assert(nuevo_slot == slot_juan); // Verificamos que se reutilizó la
        assert(pagina.get_registro(nuevo_slot) == "Pedro|30"); // Verificamos que el nuevo registro se insertó correctamente

        sm.writePage(id_pagina_asignada, pagina);
        uint16_t espacio_libre = pagina.header.espacio_libre_hacia_arriba - pagina.header.espacio_libre_hacia_abajo;
        sm.actualizar_espacio(id_pagina_asignada, espacio_libre);
        sm.guardar_mapa();
    }
    cout<<"paso la cuarta prueba";
    return 0;

}