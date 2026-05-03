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

    StorageManager prueba;
    vector<uint8_t> data;
    prueba.save_atomic("file", data);
}