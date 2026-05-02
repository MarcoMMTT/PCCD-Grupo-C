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

        } 



}