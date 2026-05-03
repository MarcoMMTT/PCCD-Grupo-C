#include "procesos.h"


int main (int argc, char* argv[]){

    if(argc != 2){
        printf("Uso correcto: ./anulaciones id_nodo\n");
        exit(EXIT_FAILURE);
    }

    struct timeval timeInicio, timeSC, timeFinSC, timeFin;
    
    FILE* ficheroSalida = fopen("salida.txt", "a");

    int mi_id = atoi(argv[1]);

    memoria_nodo *mem = NULL;

    int memoria_id = shmget(mi_id, sizeof(memoria_nodo), 0);

    mem = shmat(memoria_id, NULL, 0);

    #ifdef __DEBUG
    printf("El id de la memoria compartida es: %d\n", memoria_id);
    #endif

    #ifdef __PRINT_PROCESO
    printf("Soy un proceso de anulación\n");
    #endif

    gettimeofday(&timeInicio, NULL);

    sem_wait(&(mem->sem_contador_anul_pendientes));
    mem->contador_anul_pendientes++;
    sem_wait(&(mem->sem_testigo));
    sem_wait(&(mem->sem_turno_ANUL));
    sem_wait(&(mem->sem_turno));
    sem_wait(&(mem->sem_contador_procesos_max_SC));

    if ((!mem->testigo && (mem->contador_anul_pendientes == 1)) ||
        (mem->testigo && mem->turno_ANUL && (mem->contador_anul_pendientes + mem->contador_procesos_max_SC - EVITAR_RETENCION == 1)) ||
        (mem->testigo && (mem->contador_anul_pendientes == 1) && !mem->turno_ANUL && mem->turno)){

            // Pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_ANUL));
            sem_post(&(mem->sem_contador_procesos_max_SC));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_anul_pendientes));

            sem_wait(&(mem->sem_turno_CONS));  
            mem->turno_CONS = 0;
            sem_post(&(mem->sem_turno_CONS));
            
            sem_wait(&(mem->sem_prioridad_maxima));
            mem->prioridad_maxima = ANUL;
            sem_post(&(mem->sem_prioridad_maxima));

            #ifdef __PRINT_PROCESO
            printf("ANULACIONES --> Tengo que pedir el testigo.\n");
            #endif

            //Enviamos peticiones
            send_peticiones(mi_id, mem, ANUL);

            //Acabamos con el envio de peticiones
            sem_wait(&(mem->sem_anul_pend));

        }else{

            // No pido el testigo

            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_turno_ANUL));
            sem_post(&(mem->sem_turno));
            sem_post(&(mem->sem_contador_procesos_max_SC));
            sem_post(&(mem->sem_contador_anul_pendientes));

            #ifdef __PRINT_PROCESO
            printf("ANULACIONES --> No pido el testigo.\n");
            #endif

            sem_wait(&(mem->sem_testigo));
            sem_wait(&(mem->sem_dentro));

            if(mem->dentro || !mem->testigo){

                // Espero si hay alguien dentro o no tengo el testigo

                sem_post(&(mem->sem_dentro));
                sem_post(&(mem->sem_testigo));

                #ifdef __PRINT_PROCESO
                printf("ANULACIONES --> Espero porque no tengo permiso.\n");
                #endif

                sem_wait(&(mem->sem_anul_pend));

            }else{

                // Si no hay nadie dentro

                sem_post(&(mem->sem_dentro));
                sem_post(&(mem->sem_testigo));

                sem_wait(&(mem->sem_prioridad_maxima));
                mem->prioridad_maxima = ANUL;
                sem_post(&(mem->sem_prioridad_maxima));

                sem_wait(&(mem->sem_turno_ANUL));
                mem->turno_ANUL = 1;
                sem_post(&(mem->sem_turno_ANUL));

                sem_wait(&(mem->sem_turno));
                mem->turno = 1;
                sem_post(&(mem->sem_turno));
            }
        }

        // SECCION CRITICA

        #ifdef __PRINT_PROCESO
        printf("Proceso de anulacion en SC.\n");
        #endif

        gettimeofday(&timeSC, NULL);

        sem_wait(&(mem->sem_contador_anul_pendientes));
        mem->contador_anul_pendientes--;
        sem_post(&(mem->sem_contador_anul_pendientes));

        sem_wait(&(mem->sem_dentro));
        mem->dentro = 1;
        sem_post(&(mem->sem_dentro));

        sem_wait(&(mem->sem_contador_procesos_max_SC));
        mem->contador_procesos_max_SC++;
        sem_post(&(mem->sem_contador_procesos_max_SC));

        usleep(mem->tiempo_SC);
        
        gettimeofday(&timeFinSC, NULL);

        #ifdef __PRINT_PROCESO
        printf("Proceso de anulación sale de la SC.\n");
        #endif

        calcular_prioridad_maxima(mem);

        sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
        sem_wait(&(mem->sem_prioridad_maxima));

        if(mem->prioridad_maxima_otro_nodo > mem->prioridad_maxima){

            // Prioridad maxima en otro nodo

            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
            sem_post(&(mem->sem_prioridad_maxima));
            
            sem_wait(&(mem->sem_turno_ANUL));
            mem->turno_ANUL = 0;
            sem_post(&(mem->sem_turno_ANUL));

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

            if((mem->prioridad_maxima_otro_nodo == mem->prioridad_maxima) && (mem->prioridad_maxima_otro_nodo != 0)){

                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                sem_post(&(mem->sem_prioridad_maxima));
                sem_wait(&(mem->sem_contador_procesos_max_SC));
                sem_wait(&(mem->sem_contador_anul_pendientes));
                sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));

                if(mem->contador_procesos_max_SC >= EVITAR_RETENCION || (mem->contador_anul_pendientes == 0 && mem->prioridad_maxima_otro_nodo != 0)){

                    #ifdef __PRINT_PROCESO
                    printf("ANULACIONES --> Evito exclusion mutua o no hay procesos de esta prioridad en mi nodo.\n");
                    #endif

                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    sem_post(&(mem->sem_contador_anul_pendientes));
                    
                    sem_wait(&(mem->sem_turno_ANUL));
                    mem->turno_ANUL = 0;
                    sem_post(&(mem->sem_turno_ANUL));

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
                    sem_post(&(mem->sem_contador_anul_pendientes));
                    sem_post(&(mem->sem_anul_pend));
                }

            }else{

                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));

                if(mem->prioridad_maxima != 0){

                    if(mem->prioridad_maxima == ANUL){

                        // La prioridad mas alta de mi nodo es Anulaciones

                        #ifdef __PRINT_PROCESO
                        printf("El proceso de anulacion da paso a un proceso de administracion o de pago.\n");
                        #endif

                        sem_post(&(mem->sem_prioridad_maxima));
                        sem_post(&(mem->sem_anul_pend));

                    }else{

                        if(mem->prioridad_maxima == PAG_ADM){

                            // La prioridad mas alta de mi nodo es Pagos-Administracion

                            #ifdef __PRINT_PROCESO
                            printf("El proceso de anulacion da paso a un proceso de administracion o de pago.\n");
                            #endif

                            sem_post(&(mem->sem_prioridad_maxima));
                            sem_wait(&(mem->sem_turno_ANUL));
                            mem->turno_ANUL = 0;
                            sem_post(&(mem->sem_turno_ANUL));

                            sem_wait(&(mem->sem_turno_PAG_ADM));
                            mem->turno_PAG_ADM = 1;
                            sem_post(&(mem->sem_turno_PAG_ADM));

                            sem_wait(&(mem->sem_atendidas));
                            sem_wait(&(mem->sem_peticiones));
                            mem->atendidas[mi_id - 1][PAG_ADM - 1] = mem->peticiones[mi_id - 1][PAG_ADM - 1];
                            #ifdef __DEBUG
                            printf("DEBUG --> atendidas %d, peticiones %d.\n", mem->atendidas[mi_id - 1][PAG_ADM - 1], mem->peticiones[mi_id - 1][PAG_ADM - 1]);
                            #endif
                            sem_post(&(mem->sem_atendidas));
                            sem_post(&(mem->sem_peticiones));

                            sem_wait(&(mem->sem_contador_procesos_max_SC));
                            mem->contador_procesos_max_SC = 0;
                            sem_post(&(mem->sem_contador_procesos_max_SC));

                            sem_post(&(mem->sem_pag_adm_pend));

                        }else if(mem->prioridad_maxima == RESERVA){

                            // La prioridad mas alta en mi nodo es Reservas

                            #ifdef __PRINT_PROCESO
                            printf("El proceso de anulacion da paso a un proceso de reserva.\n");
                            #endif

                            sem_post(&(mem->sem_prioridad_maxima));
                            sem_wait(&(mem->sem_turno_ANUL));
                            mem->turno_ANUL = 0;
                            sem_post(&(mem->sem_turno_ANUL));

                            sem_wait(&(mem->sem_turno_RES));
                            mem->turno_RES = 1;
                            sem_post(&(mem->sem_turno_RES));

                            sem_wait(&(mem->sem_atendidas));
                            sem_wait(&(mem->sem_peticiones));
                            mem->atendidas[mi_id - 1][RESERVA - 1] = mem->peticiones[mi_id - 1][RESERVA - 1];
                            #ifdef __DEBUG
                            printf("DEBUG --> atendidas %d, peticiones %d.\n", mem->atendidas[mi_id - 1][RESERVA - 1], mem->peticiones[mi_id - 1][RESERVA - 1]);
                            #endif
                            sem_post(&(mem->sem_atendidas));
                            sem_post(&(mem->sem_peticiones));

                            sem_wait(&(mem->sem_contador_procesos_max_SC));
                            mem->contador_procesos_max_SC = 0;
                            sem_post(&(mem->sem_contador_procesos_max_SC));

                            sem_post(&(mem->sem_res_pend));

                        }else{

                            // La prioridad mas alta en mi nodo es Consultas

                            #ifdef __PRINT_PROCESO
                            printf("El proceso de anulacion da paso a un proceso de consulta.\n");
                            #endif

                            sem_post(&(mem->sem_prioridad_maxima));

                            sem_wait(&(mem->sem_contador_procesos_max_SC));
                            mem->contador_procesos_max_SC = 0;
                            sem_post(&(mem->sem_contador_procesos_max_SC));

                            sem_wait(&(mem->sem_turno_ANUL));
                            mem->turno_ANUL = 0;
                            sem_post(&(mem->sem_turno_ANUL));

                            sem_wait(&(mem->sem_turno_CONS));
                            mem->turno_CONS = 1;
                            sem_post(&(mem->sem_turno_CONS));

                            sem_wait(&(mem->sem_atendidas));
                            sem_wait(&(mem->sem_peticiones));
                            mem->atendidas[mi_id - 1][CONSULTA - 1] = mem->peticiones[mi_id - 1][CONSULTA - 1];
                            #ifdef __DEBUG
                            printf("DEBUG --> atendidas %d, peticiones %d.\n", mem->atendidas[mi_id - 1][CONSULTA - 1], mem->peticiones[mi_id - 1][CONSULTA - 1]);
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

                    }

                }else{
                    
                    sem_post(&(mem->sem_prioridad_maxima));

                    sem_wait(&(mem->sem_atendidas));
                    sem_wait(&(mem->sem_peticiones));
                    mem-> atendidas[mi_id - 1][ANUL - 1] = mem->peticiones[mi_id - 1][ANUL - 1];
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));

                    sem_wait(&(mem->sem_turno_ANUL));
                    mem->turno_ANUL = 0;
                    sem_post(&(mem->sem_turno_ANUL));

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

        fprintf(ficheroSalida, "%d,Anulaciones,%d,%d\n", mi_id, microsSC, microsSalida);

        return 0;

}