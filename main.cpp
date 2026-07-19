#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "storage_manager.hpp"
#include "buffer_pool_manager.hpp"
#include "b_plus_tree.hpp"

using namespace std;

int main() {
    cout << "===========================================================" << endl;
    cout << "===   SISTEMA DE GESTION DE BASES DE DATOS (B+ TREE)    ===" << endl;
    cout << "===========================================================" << endl;

    StorageManager* sm = new StorageManager("demo_storage.bin");
    BufferPoolManager* bpm = new BufferPoolManager(50, sm); 
    BPlusTree<int> tree(bpm);

    // Recuperación de estado
    std::ifstream meta_in("meta.txt");
    if (meta_in.is_open()) {
        int root_id, next_id;
        if (meta_in >> root_id >> next_id) {
            tree.SetRootPageId(root_id);
            bpm->SetNextPageId(next_id);
            cout << "[SISTEMA] Base de datos cargada. Raiz: " << root_id << endl;
        }
        meta_in.close();
    } else {
        bpm->SetNextPageId(100); // Reservamos 0-99 para textos
        cout << "[SISTEMA] Base de datos nueva. Iniciando desde cero." << endl;
    }

    int opcion;
    while (true) {
        cout << "\n--- MENU PRINCIPAL ---" << endl;
        cout << "1. Insertar registro" << endl;
        cout << "2. Buscar registro por ID" << endl;
        cout << "3. Borrar registro por ID" << endl;
        cout << "4. Panel de Control (RAM + Arbol)" << endl;
        cout << "5. Ver todos los textos (Escaneo Secuencial)" << endl;
        cout << "6. Salir y Guardar" << endl;
        cout << "Elige una opcion: ";
        cin >> opcion;

        if (opcion == 1) {
            int id; string texto;
            cout << "ID: "; cin >> id;
            cout << "Texto: "; cin.ignore(); getline(cin, texto);
            
            // 1. Guardar dato físico
            int pid = sm->encontrar_pagina_con_espacio(texto.length());
            if (pid == -1) { cout << "[ERROR] Disco lleno." << endl; continue; }
            Page* p = bpm->FetchPage(pid);
            int sid = p->insertar_registro(texto);
            sm->actualizar_espacio(pid, p->header.espacio_libre_hacia_arriba - p->header.espacio_libre_hacia_abajo);
            bpm->UnpinPage(pid, true);
            
            // 2. Guardar en Indice
            if (tree.Insert(id, RID(pid, sid))) cout << "[EXITO] Guardado exitosamente." << endl;
            else cout << "[ERROR] Error en insercion (ID duplicado o fallo interno)." << endl;

        } else if (opcion == 2) {
            int id;
            cout << "ID a buscar: "; cin >> id;
            RID result;
            if (tree.GetValue(id, &result)) {
                Page* p = bpm->FetchPage(result.page_id);
                cout << "[ENCONTRADO]: '" << p->get_registro(result.slot_id) << "'" << endl;
                bpm->UnpinPage(result.page_id, false);
            } else {
                cout << "[ERROR] ID no encontrado." << endl;
            }

        } else if (opcion == 3) {
            int id;
            cout << "ID a borrar: "; cin >> id;
            RID result;
            // 1. Buscamos la coordenada
            if (tree.GetValue(id, &result)) {
                // 2. Borramos el dato físico
                Page* p = bpm->FetchPage(result.page_id);
                p->borrar_registro(result.slot_id);
                bpm->UnpinPage(result.page_id, true);
                // 3. Borramos del índice
                tree.Remove(id);
                cout << "[EXITO] Registro borrado." << endl;
            } else {
                cout << "[ERROR] ID no encontrado." << endl;
            }

        } else if (opcion == 4) {
            cout << "\n--- PANEL DE CONTROL ---" << endl;
            bpm->MostrarEstado(); 
            cout << "\n--- ESTRUCTURA ARBOL B+ ---" << endl;
            tree.PrintTree();

        } else if (opcion == 5) {
            cout << "\n--- TEXTOS ALMACENADOS ---" << endl;
            for(int i = 0; i < 100; i++) {
                Page* p = bpm->FetchPage(i);
                for(int j = 0; j < p->header.num_slots; j++) {
                    string t = p->get_registro(j);
                    if(t != "registro borrado" && t != "Slot Id inválido") 
                        cout << "Pag " << i << " Slot " << j << ": " << t << endl;
                }
                bpm->UnpinPage(i, false);
            }

        } else if (opcion == 6) {
            // Guardado seguro
            std::ofstream meta_out("meta.txt");
            meta_out << tree.GetRootPageId() << " " << bpm->GetNextPageId();
            meta_out.close();
            sm->guardar_mapa();
            delete bpm; delete sm;
            cout << "[SISTEMA] Apagado exitoso." << endl;
            break;
        }
    }
    return 0;
}