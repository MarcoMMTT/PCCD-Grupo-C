/**
 * @file reservas.c
 * @brief Proceso de gestión de reservas (Escritor - Prioridad 2)
 * Implementa Algoritmo de Ricart-Agrawala con paso de testigo usando procesos.h
 */

#include "procesos.h"
#include <sys/time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <id_nodo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mi_id = atoi(argv[1]);
    struct timeval timeInicio, timeSC, timeFinSC, timeFin;
    FILE* fichero_salida = fopen("salida.txt", "a");

    // Vincular la memoria compartida
    int memoria_id = shmget(mi_id, sizeof(memoria_nodo), 0);
    if (memoria_id == -1) {
        perror("Error: No se pudo conectar a la memoria compartida.");
        return EXIT_FAILURE;
    }
    memoria_nodo *mem = (memoria_nodo *)shmat(memoria_id, NULL, 0);

    gettimeofday(&timeInicio, NULL);
    
    // Incrementar el contador de procesos de reserva pendientes en el nodo
    sem_wait(&(mem->sem_contador_res_pendientes));
    mem->contador_res_pendientes++;
    //sem_post(&(mem->sem_contador_res_pendientes)); ESTO TIENE QUE ESTAR EN CUALQUIER CASO DESPUES DE USARSE

    if (mem->contador_res_pendientes == 1) { 
        sem_post(&(mem->sem_contador_res_pendientes));
        // Tenemos el testigo en el nodo y nadie está en la Sección Crítica
        sem_wait(&(mem->sem_testigo));              //
        if (!(mem->testigo)) {                      // SC: mem->testigo
            sem_post(&(mem->sem_testigo));          //
            calcular_prioridad_maxima(mem);         
            send_peticiones(mi_id, mem, RESERVA);   
        } else {                                    
            sem_post(&(mem->sem_testigo));          // SC: mem->testigo
        }

        // -------------------------------------------------------------------------
        // ESPERA Y ENTRADA A SECCIÓN CRÍTICA
        // -------------------------------------------------------------------------

        sem_wait(&(mem->sem_res_pend)); // Tienes token y es tu turno                               

        #ifdef __PRINT_PROCESO
        printf("El proceso de Reservas accede a la SC.\n");
        #endif

        sem_wait(&(mem->sem_dentro));       //
                                            //
        gettimeofday(&timeSC, NULL);        //
                                            //
        mem->dentro = 1;                    // SC: SECCIÓN CRÍTICA
        usleep(mem->tiempo_SC);             //
                                            //  
                                            //
        gettimeofday(&timeFinSC, NULL);     //
                                            //
        mem->dentro = 0;                    //
                                            //
        sem_post(&(mem->sem_dentro));       //

        #ifdef __PRINT_PROCESO
        printf("El proceso de Consulta sale de la SC.\n");
        #endif

    } else {
        sem_post(&(mem->sem_contador_res_pendientes));
    
        // -------------------------------------------------------------------------
        // ESPERA Y ENTRADA A SECCIÓN CRÍTICA
        // -------------------------------------------------------------------------

        sem_wait(&(mem->sem_res_pend)); // Tienes token y es tu turno                               

        #ifdef __PRINT_PROCESO
        printf("El proceso de Reservas accede a la SC.\n");
        #endif

        sem_wait(&(mem->sem_contador_res_pendientes));
        mem->contador_res_pendientes--;
        sem_post(&(mem->sem_contador_res_pendientes));

        sem_wait(&(mem->sem_dentro));       //
                                            //
        gettimeofday(&timeSC, NULL);        //
                                            //
        mem->dentro = 1;                    // SC: SECCIÓN CRÍTICA
        usleep(mem->tiempo_SC);             //
                                            //  
                                            //
        gettimeofday(&timeFinSC, NULL);     //
                                            //
        mem->dentro = 0;                    //
                                            //
        sem_post(&(mem->sem_dentro));       //

        #ifdef __PRINT_PROCESO
        printf("El proceso de Consulta sale de la SC.\n");
        #endif
    }

    calcular_prioridad_maxima(mem);

    sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
    sem_wait(&(mem->sem_prioridad_maxima));
    
    if(mem->prioridad_maxima_otro_nodo > mem->prioridad_maxima){ // Prioridad máxima en otro nodo
        sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
        sem_post(&(mem->sem_prioridad_maxima));
        
        sem_wait(&(mem->sem_turno_RES));
        mem->turno_RES = 0;
        sem_post(&(mem->sem_turno_RES));
        
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
        if((mem->prioridad_maxima_otro_nodo == mem->prioridad_maxima) && mem->prioridad_maxima_otro_nodo != 0){
            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
            sem_post(&(mem->sem_prioridad_maxima));
            
            sem_wait(&(mem->sem_contador_procesos_max_SC));
            sem_wait(&(mem->sem_contador_res_pendientes));
            sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
            
            // Usamos EVITAR_RETENCION y contador_res_pendientes de la nueva cabecera
            if (mem->contador_procesos_max_SC >= EVITAR_RETENCION || (mem->contador_res_pendientes == 0 && mem->prioridad_maxima_otro_nodo != 0)){
                #ifdef __PRINT_PROCESO
                printf("RESERVAS --> Quiero evitar la exclusión mutua o ya no hay procesos de esta prioridad en mi nodo.\n");
                #endif
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                sem_post(&(mem->sem_contador_procesos_max_SC));
                sem_post(&(mem->sem_contador_res_pendientes));
                
                sem_wait(&(mem->sem_turno_RES));
                mem->turno_RES = 0;
                sem_post(&(mem->sem_turno_RES));
                
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
                sem_post(&(mem->sem_contador_res_pendientes));
                sem_post(&(mem->sem_res_pend));
            }
        }else{
            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
            
            if(mem->prioridad_maxima != 0){
                // Adaptación a los 4 nuevos niveles de prioridad
                if (mem->prioridad_maxima == ANUL){ 
                    
                    #ifdef __PRINT_PROCESO
                    printf("RESERVAS --> le doy paso a ANULACIONES.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima));
                    
                    sem_wait(&(mem->sem_atendidas));
                    sem_wait(&(mem->sem_peticiones));
                    mem->atendidas[mi_id - 1][ANUL - 1] = mem->peticiones[mi_id - 1][ANUL - 1];
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));
                    
                    sem_wait(&(mem->sem_contador_procesos_max_SC));
                    mem->contador_procesos_max_SC = 0;
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    
                    sem_wait(&(mem->sem_turno_RES));
                    mem->turno_RES = 0;
                    sem_post(&(mem->sem_turno_RES));
                    
                    sem_wait(&(mem->sem_turno_ANUL));
                    mem->turno_ANUL = 1;
                    sem_post(&(mem->sem_turno_ANUL));
                    
                    sem_post(&mem->sem_anul_pend);
                    
                }else if (mem->prioridad_maxima == PAG_ADM){
                    
                    #ifdef __PRINT_PROCESO
                    printf("RESERVAS --> le doy paso a PAGOS y ADMINISTRACIÓN.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima));
                    
                    sem_wait(&(mem->sem_atendidas));
                    sem_wait(&(mem->sem_peticiones));
                    mem->atendidas[mi_id - 1][PAG_ADM - 1] = mem->peticiones[mi_id - 1][PAG_ADM - 1];
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));
                    
                    sem_wait(&(mem->sem_contador_procesos_max_SC));
                    mem->contador_procesos_max_SC = 0;
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    
                    sem_wait(&(mem->sem_turno_RES));
                    mem->turno_RES = 0;
                    sem_post(&(mem->sem_turno_RES));
                    
                    sem_wait(&(mem->sem_turno_PAG_ADM));
                    mem->turno_PAG_ADM = 1;
                    sem_post(&(mem->sem_turno_PAG_ADM));
                    
                    sem_post(&mem->sem_pag_adm_pend);

                }else if (mem->prioridad_maxima == RESERVA){
                    
                    #ifdef __PRINT_PROCESO
                    printf("RESERVAS --> mantengo el turno para otra reserva.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima));
                    sem_post(&(mem->sem_res_pend));
                    
                }else{ // La prioridad mas alta de mi nodo es CONSULTA
                    
                    #ifdef __PRINT_PROCESO
                    printf("RESERVAS --> le doy paso a CONSULTAS.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima));
                    
                    sem_wait(&(mem->sem_contador_procesos_max_SC));
                    mem->contador_procesos_max_SC = 0;
                    sem_post(&(mem->sem_contador_procesos_max_SC));
                    
                    sem_wait(&(mem->sem_turno_RES));
                    mem->turno_RES = 0;
                    sem_post(&(mem->sem_turno_RES));
                    
                    sem_wait(&(mem->sem_turno_CONS));
                    mem->turno_CONS = 1; // ¡Corregido! Antes tenías mem->turno_PA = 1
                    sem_post(&(mem->sem_turno_CONS));
                    
                    sem_wait(&(mem->sem_atendidas));
                    sem_wait(&(mem->sem_peticiones));
                    mem->atendidas[mi_id - 1][CONSULTA - 1] = mem->peticiones[mi_id - 1][CONSULTA - 1];
                    #ifdef __DEBUG
                    printf("\tDEBUG --> atendidas %d, peticiones %d.\n",mem->atendidas[mi_id - 1][CONSULTA - 1], mem->peticiones[mi_id - 1][CONSULTA - 1]);
                    #endif
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));
                    
                    sem_wait(&(mem->sem_nodo_master));
                    mem->nodo_master = 1;
                    sem_post(&(mem->sem_nodo_master));
                    
                    int i;
                    sem_wait(&(mem->sem_contador_cons_pendientes));
                    for(i = 0; i < mem->contador_cons_pendientes; i++){
                        sem_post(&(mem->sem_cons_pend));
                    }
                    sem_post(&(mem->sem_contador_cons_pendientes));
                }
            }else{ // Si la prioridad máxima es 0 (no hay peticiones)
                sem_post(&(mem->sem_prioridad_maxima));
                
                sem_wait(&(mem->sem_atendidas));
                sem_wait(&(mem->sem_peticiones));
                mem->atendidas[mi_id - 1][RESERVA - 1] = mem->peticiones[mi_id - 1][RESERVA - 1];
                sem_post(&(mem->sem_atendidas));
                sem_post(&(mem->sem_peticiones));
                
                sem_wait(&(mem->sem_turno_RES));
                mem->turno_RES = 0;
                sem_post(&(mem->sem_turno_RES));
                
                sem_wait(&(mem->sem_turno));
                mem->turno = 0;
                sem_post(&(mem->sem_turno));
            }
        }
    }

    gettimeofday (&timeFin, NULL);

    int secondsSC = (timeSC.tv_sec - timeInicio.tv_sec);
    int microsSC = ((secondsSC * 1000000) + timeSC.tv_usec) - (timeInicio.tv_usec);

    int secondsSalir = (timeFin.tv_sec - timeFinSC.tv_sec);
    int microsSalir= ((secondsSalir * 1000000) + timeFin.tv_usec) - (timeFinSC.tv_usec);

    // tiempo que tarda en entrar en la SC en microsegundos, tiempo que tarda en salir desde que sale de SC en microsegundos
    fprintf (fichero_salida, "[%i,Reservas,%i,%i]\n", mi_id ,microsSC,microsSalir);

    return 0;

}
