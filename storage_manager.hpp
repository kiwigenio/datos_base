#pragma once 
#include "page.hpp"
#include <string> 
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>   


class StorageManager {
    private: 
        std::string fileName; ;
        std::string fileName_free_space_map;
        uint16_t mapa_espacio[100];
        void inicializar_archivos();
        bool cargar_mapa();
        bool guardar_mapa(); 
        void handle_error(const std::string& msg);  
    public: 
        StorageManager(std::string name);
        bool writePage(int page_id, const Page& page);
        bool readPage(int page_id, Page& page);
};

