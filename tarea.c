#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <atomic>
using namespace std;

//Cantidad maximas según la letra 
const int CAP_PIZZAS = 4, CAP_POSTRES = 3;

int pizzas = 0, postres = 0;
sem_t pizzas_libres, pizzas_listas, mutex_pizzas;
sem_t postres_libres, postres_listas, mutex_postres;
atomic<bool> terminar(false); //la modifica el main y la leen los hilos

void* productor_pizzas(void*) {
    while (!terminar) {
        sleep(3);
        sem_wait(&pizzas_libres);          // espera lugar en la mesada
        if (terminar) break;
        sem_wait(&mutex_pizzas);
        pizzas++;
        sem_post(&mutex_pizzas);
        sem_post(&pizzas_listas);
    }
    return NULL;
}

void* productor_postres(void*) {
    while (!terminar) {
        sleep(4);
        sem_wait(&postres_libres);         // espera lugar en la heladera
        if (terminar) break;
        sem_wait(&mutex_postres);
        postres++;
        sem_post(&mutex_postres);
        sem_post(&postres_listas);
    }
    return NULL;
}

// Retira n unidades si hay suficientes. Devuelve true si pudo.
bool pedir(int n, int &contador, sem_t &mutex, sem_t &listas, sem_t &libres) {
    bool ok = false;
    sem_wait(&mutex);
    if (contador >= n) {
        for (int i = 0; i < n; i++) {
            sem_wait(&listas);             // no bloquea: ya verificamos
            sem_post(&libres);             // libera lugar para el productor
        }
        contador -= n;
        ok = true;
    }
    sem_post(&mutex);
    return ok;
}

int leer_cantidad(int cap) {
    int n;
    cout << "Cantidad (1-" << cap << "): ";
    cin >> n;
    if (n <= 0 || n > cap) {
        cout << "Cantidad invalida.\n";
        return -1;
    }
    return n;
}

int main() {
    sem_init(&pizzas_libres, 0, CAP_PIZZAS);
    sem_init(&pizzas_listas, 0, 0);
    sem_init(&mutex_pizzas, 0, 1);
    sem_init(&postres_libres, 0, CAP_POSTRES);
    sem_init(&postres_listas, 0, 0);
    sem_init(&mutex_postres, 0, 1);

    pthread_t h_pizzas, h_postres;
    pthread_create(&h_pizzas, NULL, productor_pizzas, NULL);
    pthread_create(&h_postres, NULL, productor_postres, NULL);

    int opcion = 0;
cout << "\n    BIENVENIDO A LA PIZZERIA   \n";
cout << "\n       ¿Qué quiere ordenar?   \n";
while (opcion != 4) {   // el menú se repite hasta que el usuario elige 4 (Salir)
    cout << "\n1. Pedir pizzas\n2. Pedir postres\n3. Consultar inventario\n4. Salir\n> ";
    cin >> opcion;

    switch (opcion) {
        // ---- Opción 1: pedir pizzas ----
        case 1: {
            int n = leer_cantidad(CAP_PIZZAS);   // pide y valida: positiva y no mayor a 4
            if (n > 0) {                         // n es -1 si la cantidad fue inválida
                // pedir() devuelve true si había suficientes pizzas y las retiró
                if (pedir(n, pizzas, mutex_pizzas, pizzas_listas, pizzas_libres))
                    cout << "Pedido entregado: " << n << " pizza(s).\n";
                else
                    cout << "No hay suficientes pizzas.\n";   // no se entrega nada
            }
            break;   // sin break, seguiría ejecutando el caso 2
        }

        // ---- Opción 2: pedir postres (igual que pizzas, con los datos de postres) ----
        case 2: {
            int n = leer_cantidad(CAP_POSTRES);  // positiva y no mayor a 3
            if (n > 0) {
                if (pedir(n, postres, mutex_postres, postres_listas, postres_libres))
                    cout << "Pedido entregado: " << n << " postre(s).\n";
                else
                    cout << "No hay suficientes postres.\n";
            }
            break;
        }

        // ---- Opción 3: consultar inventario ----
        case 3: {
            // Cada contador se lee DENTRO de su mutex, para no leerlo a medio modificar
            sem_wait(&mutex_pizzas);  int p = pizzas;  sem_post(&mutex_pizzas);
            sem_wait(&mutex_postres); int d = postres; sem_post(&mutex_postres);
            cout << "Pizzas: " << p << " | Postres: " << d << "\n";   // el cout va afuera del mutex
            break;
        }

        // ---- Opción 4: salir ----
        case 4:
            cout << "Cerrando pizzeria...\n";   // el while termina porque opcion == 4
            break;

        // ---- Cualquier otro número ----
        default:
            cout << "Opcion no valida.\n";
    }
}

    terminar = true;
    sem_post(&pizzas_libres);              // despierta productores bloqueados
    sem_post(&postres_libres);
    pthread_join(h_pizzas, NULL);
    pthread_join(h_postres, NULL);

    sem_destroy(&pizzas_libres);  sem_destroy(&pizzas_listas);  sem_destroy(&mutex_pizzas);
    sem_destroy(&postres_libres); sem_destroy(&postres_listas); sem_destroy(&mutex_postres);
    return 0;
}

