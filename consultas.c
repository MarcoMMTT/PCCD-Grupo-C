#include "procesos.h"
#include <sys/time.h>

int main(int argc, char* argv[]){

    if(argc != 2){
        printf("Uso correcto: ./consultas id_nodo \n");
        exit(EXIT_FAILURE);
    }

    struct timeval timeInicio, timeSC, timeFinSC, timeFin;

    FILE* fichero_salida = fopen("salida.txt", "a");

    int mi_id = atoi(argv[1]);

    memoria_nodo *mem = NULL;

    int memoria_id = shmget(mi_id, sizeof(memoria_nodo), 0);

    mem = shmat(memoria_id, NULL, 0);

    #ifdef __DEBUG
    printf("Id de memoria compartida: %d", memoria_id);
    #endif

    #ifdef __PRINT_PROCESO
    printf("Proceso de Consulta.\n");
    #endif

    sleep(3);

    gettimeofday(&timeInicio, NULL);

    sem_wait(&(mem->sem_contador_cons_pendientes));
    mem->contador_cons_pendientes++;
    sem_wait(&(mem->sem_testigo));
    sem_wait(&(mem->sem_turno_CONS));
    sem_wait(&(mem->sem_turno));

    if ((!mem->testigo && mem->contador_cons_pendientes == 1) ||
        (mem->testigo && mem->contador_cons_pendientes == 1 && !mem->turno_CONS && mem->turno)){

            // Pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_CONS));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_cons_pendientes));

            calcular_prioridad_maxima(mem);

            #ifdef __PRINT_PROCESO
            printf("El proceso de Consulta tiene que pedir el testigo.\n");
            #endif

            // Se envian las peticiones
            send_peticiones(mi_id, mem, CONSULTA);

            // Acaban de enviarse las peticiones
            sem_wait(&(mem->sem_cons_pend));

        }else{

            // No pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_CONS));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_cons_pendientes));

            #ifdef __PRINT_PROCESO
            printf("El proceso de Consulta no pide el testigo.\n");
            #endif

            sem_wait(&(mem->sem_testigo));
            sem_wait(&(mem->sem_turno_CONS));

            if ((!(mem->testigo) && !mem->turno_CONS) || (!mem->consultas_dentro)){

                // No hay nadie dentro ni tengo el testigo

                sem_post(&(mem->sem_testigo));
                sem_post(&(mem->sem_turno_CONS));

                sem_wait(&(mem->sem_contador_cons_pendientes));

                if(mem->contador_cons_pendientes == 1){

                    sem_post(&(mem->sem_contador_cons_pendientes));
                    
                    calcular_prioridad_maxima(mem);

                    sem_wait(&(mem->sem_prioridad_maxima));
                    sem_wait(&(mem->sem_testigo));

                    if(mem->testigo && mem->prioridad_maxima == CONSULTA){

                        sem_post(&(mem->sem_prioridad_maxima));
                    
                        sem_wait(&(mem->sem_turno_CONS));
                        mem->turno_CONS = 1;
                        sem_post(&(mem->sem_turno_CONS));

                        sem_wait(&(mem->sem_turno));
                        mem->turno = 1;
                        sem_post(&(mem->sem_turno));

                        sem_wait(&(mem->sem_dentro));
                        mem->dentro = 1;
                        sem_post(&(mem->sem_dentro));

                        sem_wait(&(mem->sem_nodos_con_consultas));
                        mem->nodos_con_consultas[mi_id - 1] = 1;
                        sem_post(&(mem->sem_nodos_con_consultas));

                    }else{

                        sem_post(&(mem->sem_prioridad_maxima));
                        sem_post(&(mem->sem_testigo));
                    
                        #ifdef __PRINT_PROCESO
                        printf("El proceso de Consulta tiene que esperar.\n");
                        #endif

                        sem_wait(&(mem->sem_cons_pend));

                    }
                }else{

                    sem_post(&(mem->sem_contador_cons_pendientes));

                    #ifdef __PRINT_PROCESO
                    printf("El proceso de Consulta tiene que esperar.\n");
                    #endif

                    sem_wait(&(mem->sem_cons_pend));

                }
            
            }else{

            // No hay nadie dentro

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_CONS));

        }

        // SECCION CRITICA

        #ifdef __PRINT_PROCESO
        printf("El proceso de Consulta accede a la SC.\n");
        #endif

        gettimeofday(&timeSC, NULL);

        sem_wait(&(mem->sem_consultas_dentro));
        mem->consultas_dentro++;
        sem_post(&(mem->sem_consultas_dentro));

        sleep(mem->tiempo_SC);

        gettimeofday(&timeFinSC, NULL);

        #ifdef __PRINT_PROCESO
        printf("El proceso de Consulta sale de la SC.\n");
        #endif

        sem_wait(&(mem->sem_consultas_dentro));
        mem->consultas_dentro++;
        sem_post(&(mem->sem_consultas_dentro));

        sem_wait(&(mem->sem_contador_cons_pendientes));
        mem->contador_cons_pendientes--;
        sem_post(&(mem->sem_contador_cons_pendientes));

        sem_wait(&(mem->sem_turno_CONS));
        sem_wait(&(mem->sem_contador_cons_pendientes));
        sem_wait(&(mem->sem_consultas_dentro));

        if((!mem->turno_CONS && mem->consultas_dentro) || mem->contador_cons_pendientes == 0){

            sem_post(&(mem->sem_turno_CONS));
            sem_post(&(mem->sem_contador_cons_pendientes));
            sem_post(&(mem->sem_consultas_dentro));

            #ifdef __PRINT_PROCESO
            printf("Este es el último proceso de Consulta y envía el testigo.\n");
            #endif

            sem_wait(&(mem->sem_dentro));
            mem->dentro = 0;
            sem_post(&(mem->sem_dentro));

            sem_wait(&(mem->sem_turno_CONS));
            mem->turno_CONS = 0;
            sem_post(&(mem->sem_turno_CONS));

            sem_wait(&(mem->sem_turno));
            mem->turno = 0;
            sem_post(&(mem->sem_turno));

            gestionar_fin_consulta(mi_id, mem);

            #ifdef __PRINT_PROCESO
            printf("Finaliza el proceso de Consulta.\n");
            #endif

        }else{

            sem_post(&(mem->sem_turno_CONS));
            sem_post(&(mem->sem_contador_cons_pendientes));
            sem_post(&(mem->sem_consultas_dentro));

            #ifdef __PRINT_PROCESO
            printf("Finaliza el proceso de Consulta.\n");
            #endif

        }

        gettimeofday(&timeFin, NULL);

        int secondsSC = timeSC.tv_sec - timeInicio.tv_sec;
        int microsSC = secondsSC*1000000 + timeSC.tv_usec - timeInicio.tv_usec;

        int secondsSalida = timeFin.tv_sec - timeFinSC.tv_sec;
        int microsSalida = secondsSalida*1000000 + timeFin.tv_usec - timeFinSC.tv_usec;

        fprintf(fichero_salida, "[%d,Consultas,%d,%d]", mi_id, microsSC, microsSalida);

        return 0;

}