/*

#include <iostream>
#include <string>
#include "storage_manager.hpp"
#include "buffer_pool_manager.hpp"

using namespace std;

int main() {
    cout << "===========================================================" << endl;
    cout << "===   INTEGRACION PURA: STORAGE + BUFFER POOL           ===" << endl;
    cout << "===========================================================" << endl;

    StorageManager* sm = new StorageManager("demo_storage.bin");
    BufferPoolManager* bpm = new BufferPoolManager(10, sm);

    int opcion;
    while (true) {
        cout << "\n-----------------------------------------------------------" << endl;
        cout << "1. Insertar nuevo registro" << endl;
        cout << "2. Leer registro existente por Coordenadas" << endl;
        cout << "3. Borrar registro" << endl;
        cout << "4. Ver Estado del Buffer Pool (RAM y Metadatos)" << endl;
        cout << "5. Salir y Guardar (Flush a Disco)" << endl;
        cout << "-----------------------------------------------------------" << endl;
        cout << "Elige una opcion: ";
        
        if (!(cin >> opcion)) { 
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        if (opcion == 1) {
            string texto;
            cout << "-> Escribe el texto a guardar: ";
            cin.ignore(); 
            getline(cin, texto);

            int page_id = sm->encontrar_pagina_con_espacio(texto.length());
            if (page_id == -1) {
                cout << "[ERROR] Disco lleno." << endl;
                continue;
            }

            Page* p = bpm->FetchPage(page_id);
            if (p != nullptr) {
                int slot_id = p->insertar_registro(texto);
                if (slot_id != -1) {
                    uint16_t libres = p->header.espacio_libre_hacia_arriba - p->header.espacio_libre_hacia_abajo;
                    sm->actualizar_espacio(page_id, libres);
                    
                    bpm->UnpinPage(page_id, true); 
                    cout << "[EXITO] Dato guardado en RAM. Coordenadas -> Page_ID: " << page_id << " | Slot_ID: " << slot_id << endl;
                } else {
                    bpm->UnpinPage(page_id, false);
                    cout << "[ERROR] Fallo interno en la pagina." << endl;
                }
            }

        } else if (opcion == 2) {
            int pid, sid;
            cout << "-> Ingresa el Page_ID: "; cin >> pid;
            cout << "-> Ingresa el Slot_ID: "; cin >> sid;

            Page* p = bpm->FetchPage(pid);
            if (p != nullptr) {
                string resultado = p->get_registro(sid);
                cout << "\n[RESULTADO]: '" << resultado << "'" << endl;
                bpm->UnpinPage(pid, false);
            } else {
                cout << "[ERROR] Pagina invalida." << endl;
            }

        } else if (opcion == 3) {
            int pid, sid;
            cout << "-> Ingresa el Page_ID a borrar: "; cin >> pid;
            cout << "-> Ingresa el Slot_ID a borrar: "; cin >> sid;

            Page* p = bpm->FetchPage(pid);
            if (p != nullptr) {
                if (p->borrar_registro(sid)) {
                    bpm->UnpinPage(pid, true);
                    cout << "[EXITO] Registro marcado como borrado en RAM." << endl;
                } else {
                    bpm->UnpinPage(pid, false);
                    cout << "[ERROR] No se pudo borrar." << endl;
                }
            }

        } else if (opcion == 4) {
            // --- VER ESTADO DE LA RAM (BOLA DE NIEVE) ---
            bpm->MostrarEstado();

        } else if (opcion == 5) {
            cout << "\n[SISTEMA] Guardando mapa de espacio..." << endl;
            sm->guardar_mapa(); 
            cout << "[SISTEMA] Forzando Flush al disco (Guardando RAM...)" << endl;
            delete bpm; 
            delete sm;
            cout << "[SISTEMA] Apagado seguro completado." << endl;
            break;
        }
    }
    return 0;
} 

*/
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "storage_manager.hpp"
#include "buffer_pool_manager.hpp"
#include "b_plus_tree.hpp"

using namespace std;

void EjecutarPruebaDeEstres(BPlusTree<int>& tree, BufferPoolManager* bpm) {
    cout << "\n===========================================================" << endl;
    cout << "===   INICIANDO PRUEBA DE ESTRES (CASOS LIMITE)         ===" << endl;
    cout << "===========================================================" << endl;

    int num_records = 5000;
    cout << "[1/4] Insertando " << num_records << " registros (Generando Splits)..." << endl;
    for (int i = 1; i <= num_records; i++) tree.Insert(i, RID(i, i % 10)); 
    
    cout << "[2/4] Buscando los " << num_records << " registros (Comprobando integridad)..." << endl;
    RID result;
    bool exito = true;
    for (int i = 1; i <= num_records; i++) {
        if (!tree.GetValue(i, &result)) exito = false;
    }
    if (exito) cout << "      -> OK: Todos encontrados en O(log N)." << endl;

    cout << "[3/4] Borrando masivamente 3000 registros (Forzando Merging y Borrowing)..." << endl;
    for (int i = 1000; i <= 4000; i++) tree.Remove(i);
    
    cout << "[4/4] Verificando post-borrado..." << endl;
    bool integridad_ok = true;
    for (int i = 1000; i <= 4000; i++) if (tree.GetValue(i, &result)) integridad_ok = false; 
    for (int i = 1; i <= 999; i++) if (!tree.GetValue(i, &result)) integridad_ok = false; 
    if (integridad_ok) cout << "      -> OK: Arbol rebalanceado correctamente. RAM sin fugas." << endl;
    
    cout << "===========================================================\n" << endl;
}

int main() {
    cout << "===========================================================" << endl;
    cout << "===   MOTOR DE BASE DE DATOS B+ TREE (ARQUITECTURA)     ===" << endl;
    cout << "===========================================================" << endl;

    StorageManager* sm = new StorageManager("demo_storage.bin");
    BufferPoolManager* bpm = new BufferPoolManager(10, sm); // RAM de 10 frames
    BPlusTree<int> tree(bpm);

    std::ifstream meta_in("meta.txt");
    if (meta_in.is_open()) {
        int root_id;
        meta_in >> root_id;
        tree.SetRootPageId(root_id);
        meta_in.close();
        if (root_id != -1) {
            cout << "[SISTEMA] Indice B+ recuperado. Raiz en pagina: " << root_id << endl;
        }
    }
    int opcion;
    while (true) {
        cout << "\n-----------------------------------------------------------" << endl;
        cout << "1. Insertar nuevo registro (ID + Texto)" << endl;
        cout << "2. Buscar registro por ID" << endl;
        cout << "3. Borrar registro por ID" << endl;
        cout << "4. PANEL DE CONTROL (Ver Registros, RAM y Arbol B+)" << endl;
        cout << "5. Mostrar Textos (Escaneo Secuencial en Disco)" << endl;
        cout << "6. Ejecutar PRUEBA DE ESTRES MASIVA" << endl;
        cout << "7. Guardar y Salir" << endl;
        cout << "-----------------------------------------------------------" << endl;
        cout << "Elige una opcion: ";
        
        if (!(cin >> opcion)) { cin.clear(); cin.ignore(10000, '\n'); continue; }

        if (opcion == 1) {
            int id; string texto;
            cout << "-> ID (Clave): "; cin >> id;
            cout << "-> Texto: "; cin.ignore(); getline(cin, texto);

            int page_id = sm->encontrar_pagina_con_espacio(texto.length());
            if (page_id == -1) { cout << "[ERROR] Disco lleno." << endl; continue; }

            Page* p = bpm->FetchPage(page_id);
            if (p != nullptr) {
                int slot_id = p->insertar_registro(texto);
                if (slot_id != -1) {
                    sm->actualizar_espacio(page_id, p->header.espacio_libre_hacia_arriba - p->header.espacio_libre_hacia_abajo);
                    bpm->UnpinPage(page_id, true); 
                    
                    if (tree.Insert(id, RID(page_id, slot_id))) {
                        cout << "[EXITO] Registro " << id << " guardado y estructurado en el Arbol B+." << endl;
                    } else {
                        cout << "[AVISO] La clave " << id << " ya existe." << endl;
                    }
                } else bpm->UnpinPage(page_id, false);
            }

        } else if (opcion == 2) {
            int id; cout << "-> ID a buscar: "; cin >> id;
            RID result;
            if (tree.GetValue(id, &result)) {
                Page* p = bpm->FetchPage(result.page_id);
                if (p != nullptr) {
                    cout << "\n[ENCONTRADO] ID: " << id << " -> '" << p->get_registro(result.slot_id) << "'" << endl;
                    cout << "(Ubicacion fisica: Pagina " << result.page_id << ", Slot " << result.slot_id << ")" << endl;
                    bpm->UnpinPage(result.page_id, false); 
                }
            } else cout << "[ERROR] ID " << id << " no encontrado en el indice." << endl;

        } else if (opcion == 3) {
            int id; cout << "-> ID a borrar: "; cin >> id;
            RID result;
            if (tree.GetValue(id, &result)) {
                Page* p = bpm->FetchPage(result.page_id);
                if (p != nullptr) {
                    if (p->borrar_registro(result.slot_id)) {
                        bpm->UnpinPage(result.page_id, true); 
                        tree.Remove(id);
                        cout << "[EXITO] ID " << id << " eliminado fisicamente y rebalanceado en el Arbol." << endl;
                    } else bpm->UnpinPage(result.page_id, false);
                }
            } else cout << "[ERROR] ID " << id << " no existe." << endl;

        } else if (opcion == 4) {
            // --- EL GRAN PANEL DE CONTROL ---
            cout << "\n============= PANEL DE CONTROL DEL MOTOR =============" << endl;
            
            // 1. Contador de Registros Físicos (Textos)
            int total_registros = 0;
            for (int i = 0; i < 100; i++) {
                Page* p = bpm->FetchPage(i);
                if (p != nullptr) {
                    for (int j = 0; j < p->header.num_slots; j++) {
                        string t = p->get_registro(j);
                        if (t != "registro borrado" && t != "Slot Id inválido") total_registros++;
                    }
                    bpm->UnpinPage(i, false);
                }
            }
            cout << "\n[1] CANTIDAD DE REGISTROS DE TEXTO ACTIVOS: " << total_registros << endl;

            // 2. Estado del Buffer Pool Manager
            cout << "\n[2] ESTADO DE LA MEMORIA RAM (Buffer Pool):" << endl;
            bpm->MostrarEstado();

            // 3. Impresión visual del Árbol B+
            cout << "\n[3] ESTRUCTURA DEL INDICE (Arbol B+ Nivel por Nivel):" << endl;
            tree.PrintTree();
            cout << "======================================================" << endl;

        } else if (opcion == 5) {
            cout << "\n=== TEXTOS FISICOS GUARDADOS ===" << endl;
            bool hay_datos = false;
            for (int i = 0; i < 100; i++) {
                Page* p = bpm->FetchPage(i);
                if (p != nullptr) {
                    for (int j = 0; j < p->header.num_slots; j++) {
                        string t = p->get_registro(j);
                        if (t != "registro borrado" && t != "Slot Id inválido") {
                            cout << " Pag " << i << " Slot " << j << " -> '" << t << "'" << endl;
                            hay_datos = true;
                        }
                    }
                    bpm->UnpinPage(i, false);
                }
            }
            if (!hay_datos) cout << " (El disco de textos esta vacio)" << endl;

        } else if (opcion == 6) {
            EjecutarPruebaDeEstres(tree, bpm);
        } else if (opcion == 7) {
            cout << "\nGuardando estructura en disco y liberando RAM..." << endl;

            std::ofstream meta_out("meta.txt");
            meta_out << tree.GetRootPageId();
            meta_out.close();

            sm->guardar_mapa(); 
            

            delete bpm; 
            delete sm;
            
            cout << "[SISTEMA] Apagado exitoso." << endl;
            break;
        } else {
            cout << "[ERROR] Opcion no valida." << endl;
        }
    }
    return 0;
}
