#pragma once
#include "tuple.hpp"

class AbstractExecutor {
public:
    virtual ~AbstractExecutor() = default;

    // Inicializa el operador (reserva recursos, reinicia estado)
    virtual void Open() = 0;

    // Devuelve la siguiente tupla, nullptr si no hay más
    virtual Tuple* Next() = 0;

    // Libera recursos
    virtual void Close() = 0;
};