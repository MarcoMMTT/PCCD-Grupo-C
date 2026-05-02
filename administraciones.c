#include "procesos.h"
#include <sys/time.h>

#define EVITAR_RETENCION 2 // Utilidad: Limitar la ejecucion de procesos ne nodo, eviando retencion de exlcusion mutua

int main(int argc, char*argv[]){

    if(argc != 2){
        printf("Uso correcto: ./administraciones id_nodo \n");
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
    printf("Proceso de administracion.\n");
    #endif

    gettimeofday(&timeInicio, NULL);

    sem_wait(&mem->sem_contador_pag_adm_pendientes);
    mem->contador_pag_adm_pendientes++;
    sem_wait(&mem->sem_testigo);
    sem_wait(&mem->sem_turno_PAG_ADM);
    sem_wait(&mem->sem_turno);
    sem_wait(&mem->sem_contador_procesos_max_SC);

    if ((!mem->testigo && mem->contador_pag_adm_pendientes == 1) ||
        (mem->testigo && mem->turno_PAG_ADM && (mem->contador_pag_adm_pendientes + mem->contador_procesos_max_SC - EVITAR_RETENCION == 1)) ||
        (mem->testigo && (mem->contador_pag_adm_pendientes == 1) && !mem->turno_PAG_ADM && mem->turno)){

            // Pido el testigo

            sem_post(&mem->sem_testigo);
            sem_post(&mem->sem_turno_PAG_ADM);
            sem_post(&mem->sem_contador_procesos_max_SC);
            sem_post(&mem->sem_turno);
            sem_post(&mem->sem_contador_pag_adm_pendientes);
            
            sem_wait(&mem->sem_turno_CONS);
            mem->turno_CONS = 0;
            sem_post(&mem->sem_turno_CONS);

            calcular_prioridad_maxima(mem);

            #ifdef __PRINT_PROCESO
            printf("El proceso de administracion tiene que pedir el testigo.\n");
            #endif

            // Se envian las peticiones
            send_peticiones(mi_id, mem, PAG_ADM);

            // Acabamos con el envio de peticiones

            sem_wait(&mem->sem_pag_adm_pend);

        }else{

            // No pido el testigo

            sem_post(&mem->sem_testigo);
            sem_post(&mem->sem_turno_PAG_ADM);
            sem_post(&mem->sem_turno);
            sem_post(&mem->sem_contador_procesos_max_SC);
            sem_post(&mem->sem_contador_pag_adm_pendientes);

            #ifdef __PRINT_PROCESO
            printf("El proceso de administracion no tiene que pedir el testigo.\n");
            #endif

            sem_wait(&mem->sem_dentro);
            sem_wait(&mem->sem_testigo);
            
            if(mem->dentro || mem->testigo){

                // Hay alguien dentro o no se tiene el testigo

                sem_post(&mem->sem_dentro);
                sem_post(&mem->sem_testigo);
                #ifdef __PRINT_PROCESO
                printf("El proceso de administracion tiene que esperar ya que no tiene permiso.\n");
                #endif
                sem_wait(&mem->sem_pag_adm_pend);

            }else{

                // No hay nadie dentro 

                sem_post(&mem->sem_dentro);
                sem_post(&mem->sem_testigo);

                sem_wait(&mem->sem_prioridad_maxima);
                mem->prioridad_maxima = PAG_ADM;
                sem_post(&mem->sem_prioridad_maxima);

                sem_wait(&mem->sem_turno_PAG_ADM);
                mem->turno_PAG_ADM = 1;
                sem_post(&mem->sem_turno_PAG_ADM);

                sem_wait(&mem->sem_turno);
                mem->turno = 1;
                sem_post(&mem->sem_turno);

            }

        }

        // SECCION CRITICA

        #ifdef __PRINT_PROCESO
        printf("Proceso de administracion accede a la SC.\n");
        #endif

        gettimeofday(&timeSC, NULL);

        sem_wait(&mem->sem_contador_pag_adm_pendientes);
        mem->contador_pag_adm_pendientes--;
        sem_post(&mem->sem_contador_pag_adm_pendientes);

        sem_wait(&mem->sem_dentro);
        mem->dentro = 1;
        sem_post(&mem->sem_dentro);

        sem_wait(&mem->sem_contador_procesos_max_SC);
        mem->contador_procesos_max_SC++;
        sem_post(&mem->sem_contador_procesos_max_SC);

        sleep(mem->tiempo_SC);

        #ifdef __PRINT_PROCESO
        printf("El proceso de administracion sale de la SC.\n");
        #endif

        gettimeofday(&timeFinSC, NULL);

        calcular_prioridad_maxima(mem);

        sem_wait(&mem->sem_prioridad_maxima_otro_nodo);
        sem_wait(&mem->sem_prioridad_maxima);
        



}