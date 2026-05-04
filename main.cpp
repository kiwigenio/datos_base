#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <fcntl.h>    
#include <unistd.h>   
using namespace std;

const int PAGE_SIZE = 4096; 

struct Page {
    int32_t page_id;
    char data[PAGE_SIZE - 4];

    Page() {
        memset(this, 0, PAGE_SIZE);
    }
};

class StorageManager {
private:
    string fileName;

    void handle_error(const string& msg) {
        cerr << msg << ": " << strerror(errno) << endl;
    }

public:
    StorageManager(string name) : fileName(name) {}

    bool writePage(int page_id, const Page& page) {
        int fd = open(fileName.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0) {
            handle_error("Error al abrir para escritura");
            return false;
        }

        off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            handle_error("Error en lseek");
            close(fd);
            return false;
        }

        ssize_t bytes_written = write(fd, &page, PAGE_SIZE);
        if (bytes_written != PAGE_SIZE) {
            handle_error("Escritura incompleta");
            close(fd);
            return false;
        }

        if (fsync(fd) < 0) {
            handle_error("Error en fsync (durabilidad fallida)");
            close(fd);
            return false;
        }

        close(fd);
        return true;
    }

    bool readPage(int page_id, Page& page) {
        int fd = open(fileName.c_str(), O_RDONLY);
        if (fd < 0) return false;

        lseek(fd, static_cast<off_t>(page_id) * PAGE_SIZE, SEEK_SET);
        read(fd, &page, PAGE_SIZE);
        
        close(fd);
        return true;
    }
};

int main() {
    StorageManager sm("motor_db.bin");

    Page p_test;
    p_test.page_id = 50; 
    strcpy(p_test.data, "Datos persistentes con fsync");

    if (sm.writePage(50, p_test)) {
        cout << "Página 50 escrita y asegurada en disco con fsync." << endl;
    }

    return 0;
}