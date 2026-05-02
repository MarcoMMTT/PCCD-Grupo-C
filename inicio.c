#include "procesos.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char *argv[]){
    if(argc < 2){
        printf("ERROR. USO: %s [archivo]\n",argv[0]);
        return 1;
    }
    char linea[100];
    char idNodo[12];
    int i,numNodos=0,numPagos=0,numAnulaciones=0,numReservas=0,numAdmin=0,numConsultas=0;
    int tiempoSC=0;
    int n =0;
    int a;

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
            //execl("./receptor", "receptor", idNodo, numNodosStr, tiempoSCStr, NULL);
            return 0;
        }
    }



    int numOperaciones = (numPagos + numAnulaciones + numReservas + numAdmin + numConsultas) * numNodos;
    int procesosHijos[numOperaciones+1];
    for(i=1; i<=numNodos;i++){
        snprintf(idNodo, sizeof(idNodo), "%d", i);

        for(a=0;a<numAnulaciones;a++){
            procesosHijos[n] = fork();
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                //execl("./anulaciones", "anulaciones", idNodo, NULL);
                return 0;
            }
            n++;
        }

        for(a=0;a<numPagos;a++){
            procesosHijos[n] = fork();
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                //execl("./pagos", "pagos", idNodo, NULL);
                return 0;
            }
            n++;
        }
        
        for(a=0;a<numAdmin;a++){
            procesosHijos[n] = fork();
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                //execl("./admin", "admin", idNodo, NULL);
                return 0;   
            }
            n++;
        }
        for(a=0;a<numReservas;a++){
            procesosHijos[n] = fork();
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                //execl("./reservas", "reservas", idNodo, NULL);
                return 0;
            }
            n++;
        }
        
        for(a=0;a<numConsultas;a++){
            procesosHijos[n] = fork();
            if(procesosHijos[n] == 0){
                //Aquí va el execl
                //execl("./consultas", "consultas", idNodo, NULL);
                return 0;
            }
            n++;
        }




        ///////////HACER RÁFAGAS DE PROCESOS.
    }
    printf("SIMULACIÓN COMENZADA\n");
    return 0;
}