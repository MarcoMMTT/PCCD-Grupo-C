#include "procesos.h"
#include <sys/time.h>

int main(int argc, char* argv[]){

    if(argc != 2){
        printf("Uso correcto: ./consultas id_nodo \n");
        exit_failure();
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

    sem_wait(&mem->sem_contador_cons_pendientes);
    mem->contador_cons_pendientes++;
    sem_wait(&mem->sem_testigo);
    sem_wait(&mem->sem_turno_CONS);
    sem_wait(&mem->sem_turno);

    if ((!mem->testigo && mem->contador_cons_pendientes == 1) ||
        (mem->testigo && mem->contador_cons_pendientes == 1 && !mem->turno_CONS && mem->turno)){

            // Pido el testigo

            sem_post(&mem->sem_testigo);
            sem_post(&mem->sem_turno_CONS);
            sem_post(&mem->sem_turno);
            sem_post(&mem->sem_contador_cons_pendientes);

            calcular_prioridad_maxima(mem);

            #ifdef __PRINT_PROCESO
            printf("El proceso de Consulta tiene que pedir el testigo.\n");
            #endif

            // Se envian las peticiones
            send_peticiones(mi_id, mem, CONSULTA);

            // Acaban de enviarse las peticiones
            sem_wait(&mem->sem_cons_pend);

        }else{

            // No pido el testigo

            sem_post(&mem->sem_testigo);
            sem_post(&mem->sem_d);

            if (!me->turno_C){

                // Hay alguien dentro o no tengo el testigo

                sem_post(&mem->sem_testigo);
                sem_post(&mem->sem_dentro);

                sem_wait(&mem->sem_contador_cons_pendientes);

                if(mem->testigo && mem->prioridad_maxima == CONSULTA){

                    sem_post(&mem->sem_prioridad_maxima);
                    
                    sem_wait(&mem->sem_turno_CONS);
                    mem->turno_CONS = 1;
                    sem_post(&mem->sem_turno_CONS);

                    sem_wait(&mem->sem_turno);
                    mem->turno = 1;
                    sem_post(&mem->sem_turno);

                    sem_wait(&mem->sem_dentro);
                    mem->dentro = 1;
                    sem_post(&mem->sem_dentro);

                    sem_wait(&mem->sem_nodos_con_consultas);
                    mem->nodos_con_consultas[mi_id - 1] = 1;
                    sem_post(&mme->sem_nodos_con_consultas);

                }else{

                    sem_post(&mem->sem_prioridad_maxima);
                    sem_post(&mem->sem_testigo);
                    
                    #ifdef __PRINT_PROCESO
                    printf("El proceso de Consulta tiene que esperar.\n");
                    #endif

                    sem_wait(&mem->sem_cons_pend);

                }

            }else{

                sem_post(&mem->sem_contador_cons_pendientes);

                #ifdef __PRINT_PROCESO
                printf("El proceso de Consulta tiene que esperar.\n");
                #endif

                sem_wait(&mem->sem_cons_pend);

            }

        }else{

            // No hay dentro

            sem_post(&mem->sem_testigo);
            sem_post(&mem->sem_dentro);

        }

}