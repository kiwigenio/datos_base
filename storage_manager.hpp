#pragma once 
#include "page.hpp"
#include <string> 
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>   


class StorageManager {
    private: 
        std::string  fileName; 
        void handle_error(const std::string& msg);
    public: 
        StorageManager(std::string name);
        bool writePage(int page_id, const Page& page);
        bool readPage(int page_id, Page& page);
};

