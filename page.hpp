#pragma once 
#include <cstring>
#include <iostream>
#include <iostream> 
#include <cstdint>

const int PAGE_SIZE = 4096;
#pragma pack(push, 1) // Asegura que no haya padding entre los campos de las estructuras
using namespace std;

struct SlotEntry{ 
    int16_t offset; // puntero hacia donde empieza la pagina 
    uint16_t length; // para calcular cuanto mide el registro de bytes
};

//metadatos de la pagina
struct PageHeader{ 
    int16_t page_id; 
    uint16_t num_slots;
    uint16_t espacio_libre_hacia_abajo; 
    uint16_t espacio_libre_hacia_arriba;

};

// Estructura de la Página Física de 4KB
struct Page {
    PageHeader header;
    char data[PAGE_SIZE - sizeof(PageHeader)];

    // Constructor
    Page(int32_t id);
    Page() = default;

    // Declaración de métodos
    int insertar_registro(const std::string& registro);
    std::string get_registro(int slot_id);
    bool borrar_registro(int slot_id);
};

#pragma pack(pop)

