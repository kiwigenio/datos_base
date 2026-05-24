#include "storage_manager.hpp"

StorageManager::StorageManager(string name) : fileName(name) {}

void StorageManager::handle_error(const string& msg) {
    cerr << msg << ": " << strerror(errno) << endl;
}

bool StorageManager::writePage(int page_id, const Page& page){ 
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


bool StorageManager::readPage(int page_id,Page& page){

    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0) return false;

    if (lseek(fd, static_cast<off_t>(page_id) * PAGE_SIZE, SEEK_SET) == (off_t)-1) {
        close(fd);
        return false;
    }

    // SOLUCIÓN: Capturamos el retorno en 'bytes_read'
    ssize_t bytes_read = read(fd, &page, PAGE_SIZE);
    close(fd);

    // Validamos que se hayan leído exactamente los 4096 bytes de la página
    if (bytes_read != PAGE_SIZE) {
        return false; // Si fue menor o hubo error, la página no es válida
    }
    return true;

}