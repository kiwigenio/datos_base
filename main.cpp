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

    Page page1(1);
    int slot_prueba1 = page1.insertar_registro("kerin|larico|22");
    int slot_prueba2 = page1.insertar_registro("santiesteban|gomez|30");
    int slot_prueba3 = page1.insertar_registro("maria|lopez|25");

    if ( sm.writePage(page1.header.page_id, page1)) {
        cout << "Página escrita exitosamente." << endl;
    } else {
        cout << "Error al escribir la página." << endl;
    }

     Page page_lectura;

     if(sm.readPage(1, page_lectura)){
        cout << " prueba 1 "<< slot_prueba1 <<" : " << page_lectura.get_registro(slot_prueba1) <<endl;
        cout << " prueba 2 "<< slot_prueba2 <<" : " << page_lectura.get_registro(slot_prueba2) <<endl;
        cout << " prueba 3 "<< slot_prueba3 <<" : " << page_lectura.get_registro(slot_prueba3) <<endl;
    }
    return 0;

}