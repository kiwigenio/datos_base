#include "storage_manager.hpp"

// constructor apra crear los archivos o simplemente cargar los valores si es que ya existen
StorageManager::StorageManager(string name){
    fileName = name; 
    fileName_free_space_map = name  + "_espacio_libre.bin";
    if ( access(fileName.c_str(), F_OK )== 0 && access(fileName_free_space_map.c_str(),F_OK) == 0) { 
        cout<< "los archivos si existen"<<endl;
        cargar_mapa();
    }
    else { 
        cout<< "los archvios no existen (base de datos y mapa de espacio libre)"<<endl;
        inicializar_archivos();
    }
}

void StorageManager::inicializar_archivos() {
    for( int i = 0 ; i < 100; i++ ){ 
        mapa_espacio[i] = 4088;
    }
    guardar_mapa();

    Page pagina_iteradora(0); 
    for (int i = 1 ; i<100; i++ ) { 
        pagina_iteradora.header.page_id = i; 
        writePage(i, pagina_iteradora);
    }
    cout<<" se construyeron las 100 paginas vacias ye l archivo de espacio libre";
}

bool StorageManager::guardar_mapa(){
    int fd = open(fileName_free_space_map.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd< 0) { 
        handle_error("Error al abrir el mapa de espacio para escritura");
        return false;
    }
    ssize_t bytes_written = write( fd, mapa_espacio, sizeof(mapa_espacio));

    if (bytes_written != sizeof(mapa_espacio )){ 
        handle_error("Error al escribir el mapa de espacio");
        close(fd);
        return false;
    }
    fsync(fd);
    close(fd);
    return true;
}

bool StorageManager::cargar_mapa(){
    int fd = open(fileName_free_space_map.c_str(), O_RDONLY);
    if( fd < 0) {
        handle_error("Error al abrir el mapa de espacio para lectura");
        return false;
    }
    ssize_t bytes_read = read(fd, mapa_espacio, sizeof(mapa_espacio));

    if(bytes_read != sizeof(mapa_espacio)){
        handle_error("Error al leer el mapa de espacio");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}


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