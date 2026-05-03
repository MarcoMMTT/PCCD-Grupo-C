#include "procesos.h"
#include <sys/time.h>


int main(int argc, char*argv[]){

    if(argc != 2){
        printf("Uso correcto: ./pagos id_nodo \n");
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
    printf("Proceso de Pago.\n");
    #endif

    gettimeofday(&timeInicio, NULL);

    sem_wait(&(mem->sem_contador_pag_adm_pendientes));
    mem->contador_pag_adm_pendientes++;
    sem_wait(&(mem->sem_testigo));
    sem_wait(&(mem->sem_turno_PAG_ADM));
    sem_wait(&(mem->sem_turno));
    sem_wait(&(mem->sem_contador_procesos_max_SC));

    if ((!mem->testigo && mem->contador_pag_adm_pendientes == 1) ||
        (mem->testigo && mem->turno_PAG_ADM && (mem->contador_pag_adm_pendientes + mem->contador_procesos_max_SC - EVITAR_RETENCION == 1)) ||
        (mem->testigo && (mem->contador_pag_adm_pendientes == 1) && !mem->turno_PAG_ADM && mem->turno)){

            // Pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_PAG_ADM));
            sem_post(&(mem->sem_contador_procesos_max_SC));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_pag_adm_pendientes));
            
            sem_wait(&(mem->sem_turno_CONS));
            mem->turno_CONS = 0;
            sem_post(&(mem->sem_turno_CONS));

            calcular_prioridad_maxima(mem);

            #ifdef __PRINT_PROCESO
            printf("El proceso de pago tiene que pedir el testigo.\n");
            #endif

            // Se envian las peticiones
            send_peticiones(mi_id, mem, PAG_ADM);

            // Acabamos con el envio de peticiones

            sem_wait(&(mem->sem_pag_adm_pend));

        }else{

            // No pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_PAG_ADM));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_procesos_max_SC));
            sem_post(&(mem->sem_contador_pag_adm_pendientes));

            #ifdef __PRINT_PROCESO
            printf("El proceso de pago no tiene que pedir el testigo.\n");
            #endif

            sem_wait(&(mem->sem_dentro));
            sem_wait(&(mem->sem_testigo));
            
            if(mem->dentro || !mem->testigo){

                // Hay alguien dentro o no se tiene el testigo

                sem_post(&(mem->sem_dentro));
                sem_post(&(mem->sem_testigo));
                #ifdef __PRINT_PROCESO
                printf("El proceso de pago tiene que esperar ya que no tiene permiso.\n");
                #endif
                sem_wait(&(mem->sem_pag_adm_pend));

            }else{

                // No hay nadie dentro 

                sem_post(&(mem->sem_dentro));
                sem_post(&(mem->sem_testigo));

                sem_wait(&(mem->sem_prioridad_maxima));
                mem->prioridad_maxima = PAG_ADM;
                sem_post(&(mem->sem_prioridad_maxima));

                sem_wait(&(mem->sem_turno_PAG_ADM));
                mem->turno_PAG_ADM = 1;
                sem_post(&(mem->sem_turno_PAG_ADM));

                sem_wait(&(mem->sem_turno));
                mem->turno = 1;
                sem_post(&(mem->sem_turno));

            }

        }

        // SECCION CRITICA

        #ifdef __PRINT_PROCESO
        printf("Proceso de pago accede a la SC.\n");
        #endif

        gettimeofday(&timeSC, NULL);

        sem_wait(&(mem->sem_contador_pag_adm_pendientes));
        mem->contador_pag_adm_pendientes--;
        sem_post(&(mem->sem_contador_pag_adm_pendientes));

        sem_wait(&(mem->sem_dentro));
        mem->dentro = 1;
        sem_post(&(mem->sem_dentro));

        sem_wait(&(mem->sem_contador_procesos_max_SC));
        mem->contador_procesos_max_SC++;
        sem_post(&(mem->sem_contador_procesos_max_SC));

        usleep(mem->tiempo_SC);

        gettimeofday(&timeFinSC, NULL);
        
        #ifdef __PRINT_PROCESO
        printf("El proceso de pago sale de la SC.\n");
        #endif

        calcular_prioridad_maxima(mem);

        sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
        sem_wait(&(mem->sem_prioridad_maxima));

        if(mem->prioridad_maxima_otro_nodo > mem->prioridad_maxima){

            // Prioridad máxima en otro nodo

            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
            sem_post(&(mem->sem_prioridad_maxima));

            sem_wait(&(mem->sem_turno_PAG_ADM));
            mem->turno_PAG_ADM = 0;
            sem_post(&(mem->sem_turno_PAG_ADM));

            sem_wait(&(mem->sem_turno));
            mem->turno = 0;
            sem_post(&(mem->sem_turno));

            sem_wait(&(mem->sem_dentro));
            mem->dentro = 0;
            sem_post(&(mem->sem_dentro));

            sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
            if(mem->prioridad_maxima_otro_nodo == CONSULTA){
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                send_testigo_copia(mi_id, mem);
            }else{
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                send_testigo(mi_id, mem);
            }

        }else{

            if(mem->prioridad_maxima_otro_nodo == mem->prioridad_maxima && mem->prioridad_maxima_otro_nodo != 0){

                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                sem_post(&(mem->sem_prioridad_maxima));

                sem_wait(&(mem->sem_contador_procesos_max_SC));
                sem_wait(&(mem->sem_contador_pag_adm_pendientes));
                sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));

                if(mem->contador_procesos_max_SC >= EVITAR_RETENCION || (mem->contador_pag_adm_pendientes == 0 && mem->prioridad_maxima_otro_nodo != 0)){

                    #ifdef __PRINT_PROCESO
                    printf("El proceso de pago evita exclusion mutua o no hay procesos de esta prioridad en su nodo.\n");
                    #endif

                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    sem_post(&(mem->sem_contador_pag_adm_pendientes));

                    sem_wait(&(mem->sem_turno_PAG_ADM));
                    mem->turno_PAG_ADM = 0;
                    sem_post(&(mem->sem_turno_PAG_ADM));

                    sem_wait(&(mem->sem_turno));
                    mem->turno = 0;
                    sem_post(&(mem->sem_turno));

                    sem_wait(&(mem->sem_dentro));
                    mem->dentro = 0;
                    sem_post(&(mem->sem_dentro));

                    sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
                    if(mem->prioridad_maxima_otro_nodo == CONSULTA){
                        sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                        send_testigo_copia(mi_id, mem);
                    }else{
                        sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                        send_testigo(mi_id, mem);
                    }

                }else{

                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    sem_post(&(mem->sem_contador_pag_adm_pendientes));
                    sem_post(&(mem->sem_pag_adm_pend));

                }

            }else{

                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));

                if(mem->prioridad_maxima != 0){

                    if(mem->prioridad_maxima == ANUL){

                        // La prioridad más alta de mi nodo es Anulaciones

                        #ifdef __PRINT_PROCESO
                        printf("El proceso de Pago da paso a un proceso de Anulacion.\n");
                        #endif

                        sem_post(&(mem->sem_prioridad_maxima));
                        
                        sem_wait(&(mem->sem_atendidas));
                        sem_wait(&(mem->sem_peticiones));
                        mem->atendidas[mi_id - 1][ANUL - 1] = mem->peticiones[mi_id - 1][ANUL - 1];
                        #ifdef __DEBUG
                        printf("DEBUG --> atendidas %d, peticiones %d.\n", atendidas[mi_id - 1][ANUL - 1], peticiones[mi_id - 1][ANUL - 1]);
                        #endif
                        sem_post(&(mem->sem_atendidas));
                        sem_post(&(mem->sem_peticiones));

                        sem_wait(&(mem->sem_contador_procesos_max_SC));
                        mem->contador_procesos_max_SC = 0;
                        sem_post(&(mem->sem_contador_procesos_max_SC));

                        sem_wait(&(mem->sem_turno_PAG_ADM));
                        mem->turno_PAG_ADM = 0;
                        sem_post(&(mem->sem_turno_PAG_ADM));

                        sem_wait(&(mem->sem_turno_ANUL));
                        mem->turno_ANUL = 1;
                        sem_post(&(mem->sem_turno_ANUL));

                        sem_post(&(mem->sem_anul_pend));

                    }else if(mem->prioridad_maxima == PAG_ADM){

                        #ifdef __PRINT_PROCESO
                        printf("El proceso de Pago le da paso a un proceso de Administracion o Pago.\n");
                        #endif

                        sem_post(&(mem->sem_prioridad_maxima));
                        sem_post(&(mem->sem_pag_adm_pend));

                    }else if(mem->prioridad_maxima == RESERVA){

                        // La prioridad más alta de mi nodo es Reserva

                        #ifdef __PRINT_PROCESO
                        printf("El proceso de Pago da paso a un proceso de Reserva.\n");
                        #endif

                        sem_post(&(mem->sem_prioridad_maxima));
                        
                        sem_wait(&(mem->sem_atendidas));
                        sem_wait(&(mem->sem_peticiones));
                        mem->atendidas[mi_id - 1][RESERVA - 1] = mem->peticiones[mi_id - 1][RESERVA - 1];
                        #ifdef __DEBUG
                        printf("DEBUG --> atendidas %d, peticiones %d.\n", atendidas[mi_id - 1][RESERVA - 1], peticiones[mi_id - 1][RESERVA - 1]);
                        #endif
                        sem_post(&(mem->sem_atendidas));
                        sem_post(&(mem->sem_peticiones));

                        sem_wait(&(mem->sem_contador_procesos_max_SC));
                        mem->contador_procesos_max_SC = 0;
                        sem_post(&(mem->sem_contador_procesos_max_SC));

                        sem_wait(&(mem->sem_turno_PAG_ADM));
                        mem->turno_PAG_ADM = 0;
                        sem_post(&(mem->sem_turno_PAG_ADM));

                        sem_wait(&(mem->sem_turno_RES));
                        mem->turno_RES = 1;
                        sem_post(&(mem->sem_turno_RES));

                        sem_post(&(mem->sem_res_pend));

                    }else{

                        #ifdef __PRINT_PROCESO
                        printf("El proceso de Pago da paso a un proceso de Consulta.\n");
                        #endif

                        sem_post(&(mem->sem_prioridad_maxima));

                        sem_wait(&(mem->sem_contador_procesos_max_SC));
                        mem->contador_procesos_max_SC = 0;
                        sem_post(&(mem->sem_contador_procesos_max_SC));

                        sem_wait(&(mem->sem_turno_PAG_ADM));
                        mem->turno_PAG_ADM = 0;
                        sem_post(&(mem->sem_turno_PAG_ADM));

                        sem_wait(&(mem->sem_turno_CONS));
                        mem->turno_CONS = 1;
                        sem_post(&(mem->sem_turno_CONS));

                        sem_wait(&(mem->sem_atendidas));
                        sem_wait(&(mem->sem_peticiones));
                        mem->atendidas[mi_id - 1][CONSULTA - 1] = mem->peticiones[mi_id - 1][CONSULTA - 1];
                        #ifdef __DEBUG
                        printf("DEBUG --> atendidas %d, peticiones %d.\n", atendidas[mi_id - 1][CONSULTA - 1], peticiones[mi_id - 1][CONSULTA - 1]);
                        #endif
                        sem_post(&(mem->sem_atendidas));
                        sem_post(&(mem->sem_peticiones));

                        sem_wait(&(mem->sem_nodo_master));
                        mem->nodo_master = 1;
                        sem_post(&(mem->sem_nodo_master));

                        sem_wait(&(mem->sem_contador_cons_pendientes));
                        for(int i = 0; i < mem->contador_cons_pendientes; i++){
                            sem_post(&(mem->sem_cons_pend));
                        }
                        sem_post(&(mem->sem_contador_cons_pendientes));

                    }

                }else{

                    sem_post(&(mem->sem_prioridad_maxima));

                    sem_wait(&(mem->sem_atendidas));
                    sem_wait(&(mem->sem_peticiones));
                    mem-> atendidas[mi_id - 1][PAG_ADM - 1] = mem->peticiones[mi_id - 1][PAG_ADM - 1];
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));

                    sem_wait(&(mem->sem_turno_PAG_ADM));
                    mem->turno_ANUL = 0;
                    sem_post(&(mem->sem_turno_PAG_ADM));

                    sem_wait(&(mem->sem_turno));
                    mem->turno = 0;
                    sem_post(&(mem->sem_turno));

                }

            }

        }

        gettimeofday(&timeFin, NULL);

        int secondsSC = timeSC.tv_sec - timeInicio.tv_sec;
        int microsSC = secondsSC*1000000 + timeSC.tv_usec - timeInicio.tv_usec;

        int secondsSalida = timeFin.tv_sec - timeFinSC.tv_sec;
        int microsSalida = secondsSalida*1000000 + timeFin.tv_usec - timeFinSC.tv_usec;

        fprintf(fichero_salida, "[%d,Pagos,%d,%d]", mi_id, microsSC, microsSalida);

        return 0;

}