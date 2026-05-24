#include <iostream>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class StorageManager {
public:
    bool save_atomic(const string& filename, const vector<uint8_t>& data ){
        string temp_name = filename + ".tmp";

        int fd = open(temp_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return handle_error("Error al abrir archivo temporal");
        
        ssize_t bytes_written = write(fd, data.data(), data.size());
        if (bytes_written != static_cast<ssize_t>(data.size())){
            close(fd);
            return handle_error("Error al escribir");
        }

        if(fsync(fd) < 0) {
            close(fd);
            return handle_error("Error en fsync");
        }

        close(fd);

        if(rename(temp_name.c_str(), filename.c_str()) < 0){
            return handle_error("Error al renombrar");
        }

        return true;
    }
private:
    bool handle_error(const string& msg){
        cerr << msg << ": " << strerror(errno) << endl;
        return false;
    }
};

int main(){
    StorageManager sm;
    
    string mensaje = "Registro base de datos";
    vector<uint8_t> data(mensaje.begin(), mensaje.end());

    cout << "prueba de persistencia atomica" << endl;

    if (sm.save_atomic("base_datos.bin", data)) {
        cout << "[LOG] Guardado exitoso." << endl;
        cout << "[LOG] El archivo 'base_datos.bin' ha sido creado/actualizado." << endl;
    } else {
        cerr << "[ERROR] Fallo en la persistencia." << endl;
        return 1;
    }

    int fd = open("base_datos.bin", O_RDONLY);
    if (fd >= 0) {
        char buffer[100];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            cout << "[VERIFICACION] Datos en disco: " << buffer << endl;
        }
        close(fd);
    }

    return 0;
}