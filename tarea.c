#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <atomic>

using namespace std;

// Capacidades máximas según la letra
const int MAX_PIZZAS = 4;
const int MAX_POSTRES = 3;
bool ejecutando = true;
// Inventario compartido
int pizzas_disponibles = 0;
int postres_disponibles = 0;
// Semáforos y Mutex para Pizzas
sem_t sem_pizzas_vacio; 
sem_t sem_pizzas_lleno;
// Semáforos y Mutex para Postres
sem_t sem_postres_vacio;
sem_t sem_postres_lleno;
pthread_mutex_t mutex_postres;
// Hilo 1: Productor de pizzas (cada 3 segundos)
void* producir_pizzas(void* arg) {
    while (ejecutando) {
        // Espera 3 segundos simulando el horneado (FUERA de la sección crítica)
        for (int i = 0; i < 3 && ejecutando; i++) {
            sleep(3);
        }
        if (!ejecutando) break;

        // wait(empty): averigua si hay lugar en la mesada.
        // Si está llena (empty = 0), el productor se bloquea acá.
        sem_wait(&sem_pizzas_vacio);
        if (!ejecutando) break;   // me despertaron para terminar, no produzco

        // wait(mutex): solo un hilo a la vez toca el contador de pizzas
        sem_wait(&sem_mutex_pizzas);
            // ---- SECCIÓN CRÍTICA (escribir buffer) ----
            pizzas++;                    // agrego la pizza a la mesada
            int actual = pizzas;         // copia para imprimir fuera de la sección crítica
            // -------------------------------------------
        sem_post(&sem_mutex_pizzas);     // signal(mutex): libera el acceso

        // signal(full): hay una pizza más disponible para el consumidor
        sem_post(&sem_pizzas_lleno);

        cout << "Pizza lista. Pizzas en la mesada: " << actual << endl;
    }
    return NULL;
}

// Hilo 2: Productor de postres (cada 4 segundos)
void* producir_postres(void* arg) {
    while (ejecutando) {
        // Espera 4 segundos simulando la preparación (FUERA de la sección crítica)
        for (int i = 0; i < 4 && ejecutando; i++) {
            sleep(4);
        }
        if (!ejecutando) break;

        // wait(empty): ¿hay lugar en la heladera? Si está llena, se bloquea aquí
        sem_wait(&sem_postres_vacio);
        if (!ejecutando) break;   // me despertaron para terminar, no produzco

        // wait(mutex): solo un hilo a la vez toca el contador de postres
        sem_wait(&sem_mutex_postres);
            // ---- SECCIÓN CRÍTICA (escribir buffer) ----
            postres_disponibles++;
            int actual = postres_disponibles;   // copia para imprimir fuera
            // -------------------------------------------
        sem_post(&sem_mutex_postres);           // signal(mutex)

        // signal(full): hay un nuevo postre disponible
        sem_post(&sem_postres_lleno);

        cout << "Postre listo. Postres en la heladera: " << actual << endl;
    }
    return NULL;
}

// Función auxiliar para procesar pedidos de pizzas
void pedir_pizzas() {
    int cantidad;
    cout << "Ingrese cantidad de pizzas a pedir (1-" << MAX_PIZZAS << "): ";
    cin >> cantidad;

    // Validación del enunciado: positiva y no mayor a la capacidad
    if (cantidad <= 0 || cantidad > MAX_PIZZAS) {
        cout << "[Error] Cantidad invalida. Debe ser entre 1 y " << MAX_PIZZAS << ".\n";
        return;
    }

    sem_wait(&sem_mutex_pizzas);              // entro a la sección crítica
    if (pizzas_disponibles >= cantidad) {
        // Hay suficientes: las retiro una por una
        for (int i = 0; i < cantidad; i++) {
            sem_wait(&sem_pizzas_lleno);      // wait(full): consumo una pizza lista
            pizzas_disponibles--;             // SECCIÓN CRÍTICA: la saco de la mesada
            sem_post(&sem_pizzas_vacio);      // signal(empty): libero un lugar
        }
        cout << "[Exito] Se entregaron " << cantidad << " pizza(s).\n";
    } else {
        cout << "[Aviso] No hay suficientes pizzas disponibles. Stock actual: "
             << pizzas_disponibles << "\n";
    }
    sem_post(&sem_mutex_pizzas);              // salgo de la sección crítica
}

// Función auxiliar para procesar pedidos de postres
void pedir_postres() {
    int cantidad;
    cout << "Ingrese cantidad de postres a pedir (1-" << MAX_POSTRES << "): ";
    cin >> cantidad;

    if (cin.fail() || cantidad <= 0 || cantidad > MAX_POSTRES) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "[Error] Cantidad invalida. Debe ser entre 1 y " << MAX_POSTRES << ".\n";
        return;
    }

    pthread_mutex_lock(&mutex_postres);
    if (postres_disponibles >= cantidad) {
        for (int i = 0; i < cantidad; i++) {
            sem_wait(&sem_postres_lleno);
            postres_disponibles--;
            sem_post(&sem_postres_vacio); // Libera espacio en la heladera
        }
        cout << "[Exito] Se entregaron " << cantidad << " postre(s).\n";
    } else {
        cout << "[Aviso] No hay suficientes postres disponibles. Stock actual: " 
             << postres_disponibles << "\n";
    }
    pthread_mutex_unlock(&mutex_postres);
}

// Función para consultar inventario
void consultar_inventario() {
    pthread_mutex_lock(&mutex_pizzas);
    int p = pizzas_disponibles;
    pthread_mutex_unlock(&mutex_pizzas);

    pthread_mutex_lock(&mutex_postres);
    int d = postres_disponibles;
    pthread_mutex_unlock(&mutex_postres);

    cout << "\n--- INVENTARIO ACTUAL ---\n";
    cout << "Pizzas en mesada:   " << p << " / " << MAX_PIZZAS << "\n";
    cout << "Postres en heladera: " << d << " / " << MAX_POSTRES << "\n";
    cout << "-------------------------\n";
}

int main() {
    // 1. Inicializar semáforos y mutex
    sem_init(&sem_pizzas_vacio, 0, MAX_PIZZAS);
    sem_init(&sem_pizzas_lleno, 0, 0);
    pthread_mutex_init(&mutex_pizzas, nullptr);

    sem_init(&sem_postres_vacio, 0, MAX_POSTRES);
    sem_init(&sem_postres_lleno, 0, 0);
    pthread_mutex_init(&mutex_postres, nullptr);

    // 2. Crear hilos productores
    pthread_t hilo_pizzas, hilo_postres;
    pthread_create(&hilo_pizzas, nullptr, producir_pizzas, nullptr);
    pthread_create(&hilo_postres, nullptr, producir_postres, nullptr);

    // 3. Hilo principal: Menú interactivo (Consumidor)
    int opcion = 0;
    while (opcion != 4) {
        cout << "\n=== SISTEMA DE PIZZERIA ===\n";
        cout << "1. Pedir pizzas.\n";
        cout << "2. Pedir postres.\n";
        cout << "3. Consultar inventario.\n";
        cout << "4. Salir.\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[Error] Ingrese un numero valido.\n";
            continue;
        }

        switch (opcion) {
            case 1:
                pedir_pizzas();
                break;
            case 2:
                pedir_postres();
                break;
            case 3:
                consultar_inventario();
                break;
            case 4:
                cout << "Cerrando pizzeria y liberando recursos...\n";
                break;
            default:
                cout << "[Error] Opcion no valida.\n";
        }
    }

    // 4. Finalizar hilos de forma segura
    ejecutando = false;
    // Hacemos post en los semáforos vacíos por si algún productor estaba bloqueado esperando espacio
    sem_post(&sem_pizzas_vacio);
    sem_post(&sem_postres_vacio);

    pthread_join(hilo_pizzas, nullptr);
    pthread_join(hilo_postres, nullptr);

    // 5. Destruir semáforos y mutex
    sem_destroy(&sem_pizzas_vacio);
    sem_destroy(&sem_pizzas_lleno);
    pthread_mutex_destroy(&mutex_pizzas);

    sem_destroy(&sem_postres_vacio);
    sem_destroy(&sem_postres_lleno);
    pthread_mutex_destroy(&mutex_postres);

    cout << "Programa finalizado correctamente.\n";
    return 0;
}
