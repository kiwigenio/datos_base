#include "page.hpp"

Page::Page(){};

Page::Page(int32_t id ){
    memset(this, 0, sizeof(Page)); 
    header.page_id = id;
    header.num_slots = 0;
    header.espacio_libre_hacia_abajo = 0;
    header.espacio_libre_hacia_arriba = PAGE_SIZE - sizeof(PageHeader);
}

int Page::insertar_registro(const std::string& registro){
    uint16_t len = registro.length(); // tamano del registro 

    int slot_reciclado = -1;

    for ( int i = 0 ; i< header.num_slots; i++) { 
        SlotEntry* ranura_temp = reinterpret_cast<SlotEntry*> (&data[i* sizeof(SlotEntry)]);
        if ( ranura_temp-> offset == -1 ){
            slot_reciclado = i;
            break; 
        }
    }
    if(slot_reciclado == -1){
        if (header.espacio_libre_hacia_abajo + sizeof(SlotEntry) + len> header.espacio_libre_hacia_arriba ) {
        cerr << "no hay espacio suficiente para insertar el registro "<< header.page_id << endl;
        return -1; 
        }
    } else { 
        if (header.espacio_libre_hacia_abajo + len> header.espacio_libre_hacia_arriba ) {
        cerr << "no hay espacio suficiente para insertar el registro "<< header.page_id << endl;
        return -1; 
        }
    }

    header.espacio_libre_hacia_arriba -= len;
    memcpy(&data[header.espacio_libre_hacia_arriba], registro.c_str(), len);

    int slot_id_actual;
    if(slot_reciclado != -1 ) {
        SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[slot_reciclado * sizeof(SlotEntry)]);
        ranura->offset  = header.espacio_libre_hacia_arriba;
        ranura->length = len; 
        slot_id_actual = slot_reciclado;
    } else { 
        SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[header.espacio_libre_hacia_abajo]);
        ranura->offset = header.espacio_libre_hacia_arriba;
        ranura->length = len; 
        slot_id_actual = header.num_slots;
        header.espacio_libre_hacia_abajo += sizeof(SlotEntry);
        header.num_slots++;
    }
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

bool Page::borrar_registro(int slot_id) {
    if (slot_id < 0 || slot_id >= header.num_slots) {
        return false; 
    }
    SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[slot_id * sizeof(SlotEntry)]);
    // verificamos si ya estaba borrado desde antes
    if (ranura->offset == -1) {
        return false; 
    }
    // le ponemos el sticker de "nulo"
    ranura->offset = -1;
    ranura->length = 0;

    return true;
}

