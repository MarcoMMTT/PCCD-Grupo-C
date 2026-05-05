#include "procesos.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

void controlar_rafaga(int *lanzados_en_rafaga, int tamRafaga, int tiempoEntreRafagas) {
    if (tamRafaga <= 0) return;

    (*lanzados_en_rafaga)++;

    if (*lanzados_en_rafaga >= tamRafaga) {
        usleep(tiempoEntreRafagas);
        *lanzados_en_rafaga = 0;
    }
}


int main(int argc, char *argv[]){
    if(argc < 2){
        printf("ERROR. USO: %s [archivo]\n",argv[0]);
        return 1;
    }
    remove("salida.txt");
    char linea[100];
    char idNodo[12];
    int i,numNodos=0,numPagos=0,numAnulaciones=0,numReservas=0,numAdmin=0,numConsultas=0;
    int tiempoSC=0;
    int n =0;
    int a;
    int tamRafaga = 0;
    int tiempoEntreRafagas = 0; //En ms

    FILE *ficheroConfig = fopen(argv[1], "r");
    if (ficheroConfig == NULL) {
        printf("No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }
    while (fgets(linea, sizeof linea, ficheroConfig) != NULL && linea[0] != '\n') {
        char *eq = strchr(linea, '=');

        if (eq == NULL) {
            printf("Línea inválida: %s", linea);
            continue;
        }

        *eq = '\0';

        char *variable = linea;
        int valor = atoi(eq + 1);

        if (strcmp(variable, "numNodos") == 0) {
            numNodos = valor;
            printf("numNodos: %d\n", numNodos);
        } else if (strcmp(variable, "numPagos") == 0) {
            numPagos = valor;
            printf("numPagos: %d\n", numPagos);
        } else if (strcmp(variable, "numAnulaciones") == 0) {
            numAnulaciones = valor;
            printf("numAnulaciones: %d\n", numAnulaciones);
        } else if (strcmp(variable, "numReservas") == 0) {
            numReservas = valor;
            printf("numReservas: %d\n", numReservas);
        } else if (strcmp(variable, "numAdmin") == 0) {
            numAdmin = valor;
            printf("numAdmin: %d\n", numAdmin);
        } else if (strcmp(variable, "numConsultas") == 0) {
            numConsultas = valor;
            printf("numConsultas: %d\n", numConsultas);
        } else if (strcmp(variable, "tiempoSC") == 0) {
            tiempoSC = valor;
            printf("tiempoSC: %d\n", tiempoSC);
        } else if (strcmp(variable, "tamRafaga") == 0) {
            tamRafaga = valor;
            printf("tamRafaga: %d\n", tamRafaga);
        } else if (strcmp(variable, "tiempoEntreRafagas") == 0) {
            tiempoEntreRafagas = valor;
            printf("tiempoEntreRafagas: %d\n", tiempoEntreRafagas);
        } else {
            printf("Variable desconocida: %s\n", variable);
        }
    }
    if(numNodos <= 0 || numNodos > NUM_MAX_NODOS){
        printf("Número de nodos inválido. Debe ser entre 1 y %d.\n", NUM_MAX_NODOS);
        return 1;
    }
    if(tiempoSC <= 0){
        printf("Tiempo de sección crítica inválido. Debe ser un número positivo.\n");
        return 1;
    }
    if(numPagos < 0 || numAnulaciones < 0 || numReservas < 0 || numAdmin < 0 || numConsultas < 0){
        printf("Número de operaciones inválido. No puede ser negativo.\n");
        return 1;
    }
    if(tamRafaga < 0){
        printf("Tamaño de ráfaga inválido. No puede ser negativo.\n");
        return 1;
    }
    if(tiempoEntreRafagas < 0){
        printf("Tiempo entre ráfagas inválido. No puede ser negativo.\n");
        return 1;
    }
    fclose(ficheroConfig);
    
    char tiempoSCStr[12];
    snprintf(tiempoSCStr, sizeof(tiempoSCStr), "%d", tiempoSC);
    char numNodosStr[12];
    snprintf(numNodosStr, sizeof(numNodosStr), "%d", numNodos);

    int procesosReceptor[numNodos+1];
    for(i=1; i<=numNodos; i++){
        procesosReceptor[i] = fork();
        if(procesosReceptor[i] == 0){
            //Aquí va el execl
            snprintf(idNodo, sizeof(idNodo), "%d", i);
            execl("./receptor", "receptor", idNodo, numNodosStr, tiempoSCStr, NULL);
            return 0;
        }
    }

    sleep(3);

    int numOperaciones = (numPagos + numAnulaciones + numReservas + numAdmin + numConsultas) * numNodos;
    int procesosHijos[numOperaciones+1];

    int lanzados_en_rafaga = 0;

    struct timeval inicioSim, finSim;
    gettimeofday(&inicioSim, NULL);
    
    for(i=1; i<=numNodos;i++){
        snprintf(idNodo, sizeof(idNodo), "%d", i);

        for(a=0;a<numAnulaciones;a++){
            procesosHijos[n] = fork();
            if (procesosHijos[n] < 0) {
                perror("fork");
                exit(1);
            }
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                execl("./anulaciones", "anulaciones", idNodo, NULL);
                return 0;
            }
            n++;
            controlar_rafaga(&lanzados_en_rafaga, tamRafaga, tiempoEntreRafagas);
        }
int lanzados_en_rafaga = 0;

struct timeval inicioSim, finSim;
gettimeofday(&inicioSim, NULL);

/*
 * Lanzamiento round-robin:
 * en cada vuelta se lanza 1 pago por cada nodo.
 */
int totalProcesos = numNodos * numPagos;
int lanzadosTotal = 0;
int pagosLanzadosPorNodo[NUM_MAX_NODOS] = {0};

    while (lanzadosTotal < totalProcesos) {

        int lanzadosEstaRafaga = 0;

        while (lanzadosEstaRafaga < tamRafaga && lanzadosTotal < totalProcesos) {

            int lanzado = 0;

            for (i = 1; i <= numNodos && lanzadosEstaRafaga < tamRafaga; i++) {

                if (pagosLanzadosPorNodo[i - 1] >= numPagos) {
                    continue;
                }

                snprintf(idNodo, sizeof(idNodo), "%d", i);

                procesosHijos[n] = fork();

                if (procesosHijos[n] < 0) {
                    perror("fork");
                    exit(1);
                }

                if (procesosHijos[n] == 0) {
                    execl("./pagos", "pagos", idNodo, NULL);
                    perror("execl pagos");
                    exit(1);
                }

                pagosLanzadosPorNodo[i - 1]++;
                lanzadosTotal++;
                lanzadosEstaRafaga++;
                n++;
                lanzado = 1;
            }

            if (!lanzado) {
                break;
            }
        }

        if (lanzadosTotal < totalProcesos && tiempoEntreRafagas > 0) {
            usleep(tiempoEntreRafagas * 1000); // ms -> us
        }   
    }
        
        for(a=0;a<numAdmin;a++){
            procesosHijos[n] = fork();
            if (procesosHijos[n] < 0) {
                perror("fork");
                exit(1);
            }
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                execl("./administraciones", "administraciones", idNodo, NULL);
                return 0;   
            }
            n++;
            controlar_rafaga(&lanzados_en_rafaga, tamRafaga, tiempoEntreRafagas);

        }
        for(a=0;a<numReservas;a++){
            procesosHijos[n] = fork();
            if (procesosHijos[n] < 0) {
                perror("fork");
                exit(1);
            }
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                execl("./reservas", "reservas", idNodo, NULL);
                return 0;
            }
            n++;
            controlar_rafaga(&lanzados_en_rafaga, tamRafaga, tiempoEntreRafagas);
        }
        
        for(a=0;a<numConsultas;a++){
            procesosHijos[n] = fork();
            if (procesosHijos[n] < 0) {
                perror("fork");
                exit(1);
            }
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                execl("./consultas", "consultas", idNodo, NULL);
                return 0;
            }
            n++;
            controlar_rafaga(&lanzados_en_rafaga, tamRafaga, tiempoEntreRafagas);
        }
        
    }
    printf("SIMULACIÓN COMENZADA\n");
    for (i = 0; i < numOperaciones; i++) {
        waitpid(procesosHijos[i], NULL, 0);
    }
    printf("SIMULACIÓN TERMINADA\n");

    gettimeofday(&finSim, NULL);

    long segundos = finSim.tv_sec - inicioSim.tv_sec;
    long micros = segundos * 1000000 + finSim.tv_usec - inicioSim.tv_usec;
    double tiempo_real = micros / 1000000.0;

    // ===== CÁLCULOS =====
    int totalProcesos = numNodos * numPagos;

    // Ideal puro (sin ráfagas)
    double ideal_servicio = ((double) totalProcesos * tiempoSC) / 1000000.0;

    // Número de pausas de ráfaga
    int numPausas = 0;
    if (tamRafaga > 0) {
        numPausas = (totalProcesos - 1) / tamRafaga;
    }

    // Ideal con ráfagas (incluyendo pausas)
    double ideal_rafagas = ideal_servicio +
        ((double) numPausas * tiempoEntreRafagas) / 1000.0;

    // Sobrecostes
    double sobrecoste_algoritmo = tiempo_real - ideal_servicio;
    double sobrecoste_rafagas = tiempo_real - ideal_rafagas;

    

   FILE *f = fopen("metricas_rafagas_2.csv", "a");

// Si el fichero está vacío, escribe cabecera
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f,
            "numNodos,numPagos,totalProcesos,tiempoSC_us,"
            "tamRafaga,tiempoEntreRafagas_ms,"
            "idealServicio_s,idealRafagas_s,tiempoReal_s,"
            "sobrecosteAlgoritmo_s,sobrecosteRafagas_s\n"
        );
    }

    // Escribir datos
    fprintf(f,
        "%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        numNodos,
        numPagos,
        totalProcesos,
        tiempoSC,
        tamRafaga,
        tiempoEntreRafagas,
        ideal_servicio,
        ideal_rafagas,
        tiempo_real,
        sobrecoste_algoritmo,
        sobrecoste_rafagas
    );

    fclose(f);
    
    return 0;
}