/**
 * @file reservas.c
 * @brief Proceso de gestión de reservas (Escritor - Prioridad 2)
 * Implementa Algoritmo de Ricart-Agrawala con paso de testigo usando procesos.h
 */

#include "procesos.h"
#include <sys/time.h>


int main(int argc, char *argv[]){
    
    if (argc != 2){
        printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
        return -1;
    }
    struct timeval timeInicio, timeSC, timeFinSC, timeFin;
    
    FILE * ficheroSalida= fopen ("salida.txt", "a");
    
    int mi_id = atoi(argv[1]);
    
    memoria_nodo *mem = NULL;
    
    // inicialización memoria compartida
    int memoria_id = shmget(mi_id, sizeof(memoria_nodo), 0);
    mem = shmat(memoria_id, NULL, 0);
    
    #ifdef __DEBUG
    printf("El id de la memoria compartida es: %i\n", memoria_id);
    #endif
    
    #ifdef __PRINT_PROCESO
    printf("Proceso de Reservas.\n"); 
    #endif
    
    gettimeofday (&timeInicio, NULL);
    
    sem_wait(&(mem->sem_contador_res_pendientes));
    mem->contador_res_pendientes = mem->contador_res_pendientes + 1;
    sem_wait(&(mem->sem_testigo));
    sem_wait(&(mem->sem_turno_RES));
    sem_wait(&(mem->sem_turno));
    sem_wait(&(mem->sem_contador_procesos_max_SC));
    
    if ((!mem->testigo && (mem->contador_res_pendientes == 1)) || 
    (mem->testigo && mem->turno_RES && (mem->contador_res_pendientes + mem->contador_procesos_max_SC - EVITAR_RETENCION) == 1)
    || (mem->testigo && (mem->contador_res_pendientes == 1) && !mem->turno_RES && mem->turno)){ 
        //RAMA DE PEDIR EL TESTIGO
        sem_post(&(mem->sem_contador_res_pendientes));
        sem_post(&(mem->sem_testigo));
        sem_post(&(mem->sem_turno_RES));
        sem_post(&(mem->sem_turno));
        sem_post(&(mem->sem_contador_procesos_max_SC));
        
        sem_wait(&(mem->sem_turno_CONS));
        mem->turno_CONS = 0; // false
        sem_post(&(mem->sem_turno_CONS));
        
        calcular_prioridad_maxima(mem);
        
        #ifdef __PRINT_PROCESO
        printf("RESERVAS --> Tengo que pedir el testigo\n");
        #endif
        
        //Enviamos peticiones
        send_peticiones(mi_id, mem, RESERVA);
        
        // ACABAMOS CON EL ENVIO DE PETICIONES AHORA ME TOCA ESPERAR.
        sem_wait(&(mem->sem_res_pend));
    }else{ // NO TENGO QUE PEDIR EL TESTIGO
        sem_post(&(mem->sem_contador_res_pendientes));
        sem_post(&(mem->sem_testigo));
        sem_post(&(mem->sem_turno_RES));
        sem_post(&(mem->sem_turno));
        sem_post(&(mem->sem_contador_procesos_max_SC));
        
        #ifdef __PRINT_PROCESO
        printf("El proceso de Reserva no tiene que pedir el testigo.\n");
        #endif
        
        sem_wait(&(mem->sem_testigo));
        sem_wait(&(mem->sem_dentro));
        
        if ((mem->dentro) || !(mem->testigo)){ // SI HAY ALGUIEN DENTRO O NO TENGO EL TESTIGO, ESPERO
            
            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_dentro));
            
            #ifdef __PRINT_PROCESO
            printf("El proceso de Reserva tiene que esperar ya que no tiene permiso.\n");
            #endif
            
            sem_wait(&(mem->sem_res_pend));
        }else{ // SI NO HAY NADIE DENTRO
            sem_post(&(mem->sem_testigo));
            sem_post(&(mem->sem_dentro));
            
            sem_wait(&(mem->sem_prioridad_maxima));
            mem->prioridad_maxima = RESERVA;
            sem_post(&(mem->sem_prioridad_maxima));
            
            sem_wait(&(mem->sem_turno_RES));
            mem->turno_RES = 1; // true
            sem_post(&(mem->sem_turno_RES));
            
            sem_wait(&(mem->sem_turno));
            mem->turno = 1; // true
            sem_post(&(mem->sem_turno));
        }
    }
    
    // SECCIÓN CRÍTICA DE EXCLUSIÓN MUTUA BABY
    #ifdef __PRINT_PROCESO
    printf("Proceso de Reserva accede a la SC.\n");
    #endif
    
    gettimeofday (&timeSC, NULL);
    
    sem_wait(&(mem->sem_contador_res_pendientes));
    mem->contador_res_pendientes = mem->contador_res_pendientes - 1;
    sem_post(&(mem->sem_contador_res_pendientes));
    
    sem_wait(&(mem->sem_dentro));
    mem->dentro = 1; // true
    sem_post(&(mem->sem_dentro));
    
    sem_wait(&(mem->sem_contador_procesos_max_SC));
    mem->contador_procesos_max_SC = mem->contador_procesos_max_SC + 1;
    sem_post(&(mem->sem_contador_procesos_max_SC));
    
    usleep(mem->tiempo_SC); 
    
    #ifdef __PRINT_PROCESO
    printf("El proceso de Reserva sale de la SC.\n");
    #endif
    
    printf("hola\n");
    gettimeofday (&timeFinSC, NULL);

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
                printf("El proceso de Reserva evita exclusion mutua o no hay procesos de esta prioridad en su nodo.\n");
                #endif
                sem_post(&(mem->sem_contador_procesos_max_SC));
                sem_post(&(mem->sem_contador_res_pendientes));
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                
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
                sem_post(&(mem->sem_contador_procesos_max_SC));
                sem_post(&(mem->sem_contador_res_pendientes));
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                
                sem_post(&(mem->sem_res_pend));
            }
        }else{
            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
            
            if(mem->prioridad_maxima != 0){
                // Adaptación a los 4 nuevos niveles de prioridad
                if (mem->prioridad_maxima == ANUL){ 
                    
                    #ifdef __PRINT_PROCESO
                    printf("El proceso de Reserva da paso a un proceso de Anulacion.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
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
                    printf("El proceso de Reserva da paso a un proceso de Administración o Pago.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
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
                    printf("El proceso de Reserva mantiene el turno para otra Reserva.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
                    sem_post(&(mem->sem_prioridad_maxima));
                    sem_post(&(mem->sem_res_pend));
                    
                }else{ // La prioridad mas alta de mi nodo es CONSULTA
                    
                    #ifdef __PRINT_PROCESO
                    printf("El proceso de Reserva da paso a un proceso de Consultas.\n");
                    #endif
                    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
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
                sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
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

    long secondsSC = timeSC.tv_sec - timeInicio.tv_sec;
    long microsSC = secondsSC*1000000 + timeSC.tv_usec - timeInicio.tv_usec;

    long secondsSalida = timeFin.tv_sec - timeFinSC.tv_sec;
    long microsSalida = secondsSalida*1000000 + timeFin.tv_usec - timeFinSC.tv_usec;

    // tiempo que tarda en entrar en la SC en microsegundos, tiempo que tarda en salir desde que sale de SC en microsegundos
    fprintf (ficheroSalida, "%d,Reservas,%ld,%ld\n", mi_id , microsSC, microsSalida);

    return 0;

}
