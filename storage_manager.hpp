#pragma once 
#include "page.hpp"
#include <string> 
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>   


class StorageManager {
    private: 
        std::string fileName; // el archivo que es la base de datos 
        std::string fileName_free_space_map; // el archivo que contiene el espacio libre de cada pagina 
        uint16_t mapa_espacio[100]; // los datos de cada pagina que se guardaran cuando se suba a memoria 
        
        void handle_error(const std::string& msg);  // manejo de error al abrir, leer o escribir archivos
    public: 
        StorageManager(std::string name);
        void inicializar_archivos();
        bool cargar_mapa(); // subir el archivo a memoria
        bool guardar_mapa();  // guardar el archivo para la persistencia de datos
        bool writePage(int page_id, const Page& page); // escribir en el disco los cambios hecho o cualquier cambio en general 
        bool readPage(int page_id, Page& page); // leer de mi archivo los datos requeridos 
        int encontrar_pagina_con_espacio(uint16_t tamano_requerido); // encuentra la pagian con espacio suficiente para insertar un nuevo registro, devuelve el id de la pagina o -1 si no hay espacio
        void actualizar_espacio(int page_id, uint16_t nuevo_espacio_libre); // actualiza el espacio cuando hay registros o se elimino 
};

