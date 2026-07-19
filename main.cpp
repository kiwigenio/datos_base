#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "storage_manager.hpp"
#include "buffer_pool_manager.hpp"
#include "b_plus_tree.hpp"
#include "seq_scan_executor.hpp"
#include "select_executor.hpp"
#include "project_executor.hpp"
#include "nested_loop_join_executor.hpp"
#include "index_nested_loop_join_executor.hpp"

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
        cout << "4. PANEL DE CONTROL (Ver Registros, RAM y Arbol B+)" << endl;
        cout << "5. Mostrar Textos (Escaneo Secuencial en Disco)" << endl;
        cout << "6. Ejecutar Consulta (Volcano Model)" << endl;
        cout << "7. Ejecutar Join (Nested Loop + Index)" << endl;
        cout << "8. Insertar en Tabla B (páginas 75-99)" << endl;
        cout << "9. Ver Hit Rate del Buffer Pool" << endl;
        cout << "10. Ejecutar Prueba Exhaustiva" << endl;
        cout << "11. Guardar y Salir" << endl;
        cout << "-----------------------------------------------------------" << endl;
        cout << "Elige una opcion: ";
        cin >> opcion;

        if (opcion == 1) {
            int id; string texto;
            cout << "-> ID (Clave): "; cin >> id;
            cout << "-> Texto: "; cin.ignore(); getline(cin, texto);

            int page_id = sm->encontrar_pagina_con_espacio(texto.length(), 50, 75); // tabla A
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
            cout << "\n=== PROCESAMIENTO DE CONSULTAS (VOLCANO MODEL) ===" << endl;
            cout << "Formato de registros: columna_0 | columna_1 | columna_2 ..." << endl;

            // --- Configuración de la consulta ---
            int col_filtro, col_proyecto;
            std::string valor_filtro;

            cout << "\n-> Columna a filtrar (0, 1, 2, ...): ";
            cin >> col_filtro;
            cout << "-> Valor a buscar en esa columna: ";
            cin.ignore();
            getline(cin, valor_filtro);
            cout << "-> Columna a proyectar (-1 para todas): ";
            cin >> col_proyecto;

            // --- Armamos la pipeline ---
            // Capa 1: Scan — lee todas las páginas via BPM
            SeqScanExecutor scan(bpm, 100);

            // Capa 2: Select — filtra por la columna y valor elegidos
            SelectExecutor select(&scan, [col_filtro, valor_filtro](const Tuple& t) -> bool {
                if (col_filtro < 0 || col_filtro >= (int)t.columns.size()) return false;
                return t.columns[col_filtro] == valor_filtro;
            });

            // Capa 3: Project — proyecta la(s) columna(s) elegidas
            std::vector<int> cols_a_proyectar;
            if (col_proyecto == -1) {
                // Proyectamos todas: ejecutamos un Next para saber cuántas columnas hay
                scan.Open();
                Tuple* sample = scan.Next();
                if (sample != nullptr) {
                    for (int i = 0; i < (int)sample->columns.size(); i++) {
                        cols_a_proyectar.push_back(i);
                    }
                }
                scan.Close();
            } else {
                cols_a_proyectar.push_back(col_proyecto);
            }

            ProjectExecutor project(&select, cols_a_proyectar);

            // --- Ejecutamos la pipeline ---
            cout << "\n[SCAN → SELECT → PROJECT]" << endl;
            cout << "Filtrando columna " << col_filtro << " == \"" << valor_filtro << "\"" << endl;
            cout << "-------------------------------------------" << endl;

            project.Open();
            int count = 0;
            Tuple* result;
            while ((result = project.Next()) != nullptr) {
                result->Print();
                cout << endl;
                count++;
            }
            project.Close();

            cout << "-------------------------------------------" << endl;
            cout << "Total de tuplas encontradas: " << count << endl;
        } else if (opcion == 7) {
            cout << "\n=== JOIN DE TABLAS (Páginas 50-74 ⋈ Páginas 75-99) ===" << endl;
            cout << "1. Nested Loop Join (fuerza bruta)" << endl;
            cout << "2. Index Nested Loop Join (optimizado con B+ Tree)" << endl;
            cout << "-> Elige tipo de join: ";
            int tipo_join; cin >> tipo_join;

            int outer_col, inner_col;
            cout << "-> Columna de join en tabla izquierda: "; cin >> outer_col;

            if (tipo_join == 1) {
                cout << "-> Columna de join en tabla derecha: "; cin >> inner_col;

                SeqScanExecutor outer_scan(bpm, 50, 25);  // páginas 50-74
                SeqScanExecutor inner_scan(bpm, 75, 25);  // páginas 75-99
                NestedLoopJoinExecutor join(&outer_scan, &inner_scan, outer_col, inner_col);

                cout << "\n[NESTED LOOP JOIN]" << endl;
                cout << "-------------------------------------------" << endl;
                join.Open();
                int count = 0;
                Tuple* t;
                while ((t = join.Next()) != nullptr) {
                    t->Print(); cout << endl;
                    count++;
                }
                join.Close();
                cout << "-------------------------------------------" << endl;
                cout << "Total de tuplas joined: " << count << endl;

            } else {
                cout << "\n[INDEX NESTED LOOP JOIN] Usando B+ Tree como índice inner" << endl;
                cout << "-> La columna de join outer debe ser el ID (entero)" << endl;
                cout << "-------------------------------------------" << endl;

                SeqScanExecutor outer_scan(bpm, 50, 25);  // páginas 50-74 como outer
                IndexNestedLoopJoinExecutor index_join(&outer_scan, &tree, bpm, outer_col);

                index_join.Open();
                int count = 0;
                Tuple* t;
                while ((t = index_join.Next()) != nullptr) {
                    t->Print(); cout << endl;
                    count++;
                }
                index_join.Close();
                cout << "-------------------------------------------" << endl;
                cout << "Total de tuplas joined: " << count << endl;
            }      
        } else if (opcion == 8) {
            int id; string texto;
            cout << "-> ID (Clave): "; cin >> id;
            cout << "-> Texto (Tabla B): "; cin.ignore(); getline(cin, texto);

            int page_id = sm->encontrar_pagina_con_espacio(texto.length(), 75, 100); // tabla B
            if (page_id == -1) { cout << "[ERROR] Tabla B llena." << endl; continue; }

            Page* p = bpm->FetchPage(page_id);
            if (p != nullptr) {
                int slot_id = p->insertar_registro(texto);
                if (slot_id != -1) {
                    sm->actualizar_espacio(page_id, p->header.espacio_libre_hacia_arriba - p->header.espacio_libre_hacia_abajo);
                    bpm->UnpinPage(page_id, true);
                    cout << "[EXITO] Registro guardado en Tabla B. Page: " << page_id << " Slot: " << slot_id << endl;
                } else bpm->UnpinPage(page_id, false);
            }
        } else if (opcion == 9) {
            bpm->ReportHitRate();

        } else if (opcion == 10) {
            cout << "\n=== PRUEBA EXHAUSTIVA ===" << endl;

            cout << "\n[1] Reiniciando estadisticas..." << endl;
            bpm->ResetStats();

            cout << "[2] Prueba de localidad temporal (misma pagina 10 veces)..." << endl;
            for (int i = 0; i < 10; i++) {
                Page* p = bpm->FetchPage(50);
                if (p) bpm->UnpinPage(50, false);
            }
            bpm->ReportHitRate();  // debería ser ~90% (1 miss + 9 hits)
            bpm->ResetStats();

            cout << "\n[3] Prueba de scan secuencial (todas las paginas de datos)..." << endl;
            SeqScanExecutor scan(bpm, 50, 50);
            scan.Open();
            int count = 0;
            while (scan.Next() != nullptr) count++;
            scan.Close();
            cout << "Tuplas escaneadas: " << count << endl;
            bpm->ReportHitRate();  // hit rate bajo, muchas páginas distintas
            bpm->ResetStats();

            cout << "\n[4] Prueba de busqueda por indice B+ (10 búsquedas)..." << endl;
            RID result;
            for (int i = 1; i <= 4; i++) {
                tree.GetValue(i, &result);
            }
            bpm->ReportHitRate();  // hit rate alto, el árbol reutiliza páginas
            bpm->ResetStats();

            cout << "\n[5] Prueba de Join completo..." << endl;
            SeqScanExecutor outer(bpm, 50, 25);
            SeqScanExecutor inner(bpm, 75, 25);
            NestedLoopJoinExecutor join(&outer, &inner, 0, 0);
            join.Open();
            int join_count = 0;
            while (join.Next() != nullptr) join_count++;
            join.Close();
            cout << "Tuplas joined: " << join_count << endl;
            bpm->ReportHitRate();

            cout << "\n=== FIN DE PRUEBA EXHAUSTIVA ===" << endl;               
        } else if (opcion == 11) {
            cout << "\nGuardando estructura en disco y liberando RAM..." << endl;

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