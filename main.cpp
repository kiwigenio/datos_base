#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <fcntl.h>    
#include <unistd.h>   
using namespace std;

const int PAGE_SIZE = 4096; 

//representa la ranura del directorio 
struct SlotEntry{ 
    int16_t offset; // puntero hacia donde empieza la pagina 
    uint16_t length; // para calcular cuanto mide el registro de bytes
};

//metadatos de la pagina
struct PageHeader{ 
    int16_t page_id; 
    uint16_t num_slots;
    uint16_t espacio_libre_abajo; 
    uint16_t espacio_libre_arriba;

};


struct Page {
    PageHeader header;
    char data[PAGE_SIZE - sizeof(PageHeader)];

    Page(int32_t id= -1){
        memset(this, 0, sizeof(Page)); 
        header.page_id = id;
        header.num_slots = 0;
        header.espacio_libre_abajo = 0;
        header.espacio_libre_arriba = PAGE_SIZE - sizeof(PageHeader);
    }
    int insertar_registro(const string& registro){ 
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

    string get_registro(int slot_id) { 
        if(slot_id< 0 || slot_id >= header.num_slots){ 
            return "Slot Id inválido";
        }

        SlotEntry* ranura = reinterpret_cast<SlotEntry*>(&data[slot_id * sizeof(SlotEntry)]);
        if(ranura-> offset == -1) return "registro borrado";
        return string(&data[ranura-> offset], ranura-> length);
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
    p_test.page_id = 1; 
    strcpy(p_test.data, "Datos persistentes con fsync parte 2 ");
    Page p_test2;
    p_test2.page_id = 50; 
    strcpy(p_test2.data, "Datos para la página 50");

    /*if (sm.writePage(p_test.page_id, p_test)) {
        cout << "Página " << p_test.page_id << " escrita y asegurada en disco con fsync." << endl;
    }
    sm.readPage(p_test.page_id, p_test);
    cout << "Página leída: " << p_test.data << endl;
    */
    if (sm.writePage(p_test2.page_id, p_test2)) {
        cout << "Página " << p_test2.page_id << " escrita y asegurada en disco con fsync. parte 50" << endl;
    }
    sm.readPage(p_test2.page_id, p_test2);
    cout << "Página leída: " << p_test2.data << endl;
    return 0;
}