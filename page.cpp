#include "page.hpp"


Page::Page(int32_t id ){
    memset(this, 0, sizeof(Page)); 
    header.page_id = id;
    header.num_slots = 0;
    header.espacio_libre_abajo = 0;
    header.espacio_libre_arriba = PAGE_SIZE - sizeof(PageHeader);
}

int Page::insertar_registro(const std::string& registro){
    uint16_t len = registro.length();

    if (header.espacio_libre_abajo + sizeof(SlotEntry) + len> header.espacio_libre_arriba ) {
        cerr << "no hay espacio suficiente para insertar el registro "<< header.page_id << endl;
        return -1; 
    }
    header.espacio_libre_arriba -= len;
    memcpy(&data[header.espacio_libre_arriba], registro.c_str(), len);

    SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[header.espacio_libre_abajo]);
    ranura-> offset = header.espacio_libre_arriba; 
    ranura-> length = len; 

    int slot_id_actual = header.num_slots;
    header.espacio_libre_abajo += sizeof(SlotEntry);
    header.num_slots++;
    return slot_id_actual;  
    
}

string Page::get_registro(int slot_id){ 
    if(slot_id< 0 || slot_id >= header.num_slots){ 
        return "Slot Id inválido";
    }

    SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[slot_id * sizeof(SlotEntry)]);
    if(ranura-> offset == -1) return "registro borrado";
    return string(&data[ranura-> offset], ranura-> length);
}

