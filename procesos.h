#ifndef PROCESOS_H
#define PROCESOS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>


//----------------------------------------------------- {Ifdefs} ----------------------------------------
//- Puede ser interesantre implementar variables de condición para distintas ejecuciones                -
//-------------------------------------------------------------------------------------------------------
//#define PRINT_RX        // comentar en caso de no querer mensajes del proceso receptor
//#define PRINT_PROCESO   // comentar en caso de no querer mensajes de los procesos escritores del nodo.
//#define PRINT_CONSULTAS // comentar en caso deno querer mensajes de los procesos consultas.
//#define DEBUG
//-------------------------------------------------------------------------------------------------------------

#define NUM_MAX_NODOS 100 // Número máximo de nodos en el sistema. Se puede modificar según las necesidades del sistema.
#define P 4 // Nº de prioridades distintas: 1. Anulaciones -> 2. Pagos - Administración -> 3. Reservas -> 4. Consultas 
#define ANUL 4
#define PAG_ADM 3
#define RESERVA 2
#define CONSULTA 1

/** 
 * @brief Estructura para los mensajes.
 * @param msg_type: TIPO 1 --> SOLICITUD; TIPO 2 --> TESTIGO MAESTRO; TIPO 3 --> ENVIAR TESTIGO COPIA; TIPO 4 --> RECIBIR TESTIGO COPIA
 * @param id: ID del Nodo
 * @param peticion: Número de petición
 * @param prioridad: Prioridad de la petición
 * @param atendidas[N][P]: Matriz de peticiones
 * @param id_nodo_master: ID del nodo con el Testigo Maestro
 */
typedef struct {
  long msg_type;
  int id;
  int peticion;
  int prioridad;
  int atendidas[NUM_MAX_NODOS][P];
  int id_nodo_master;
}msgbuf_mensaje;

typedef struct {

    int num_nodos; // SE RECIBE DEL FICHERO DE CONFIG
    int tiempo_SC; // SE RECIBE DEL FICHERO DE CONFIG


    // VARIABLES QUE VAN A USAR LOS LECTORES:
    int consultas_dentro;
    int id_nodo_master;
    int testigos_recogidos;
    int id_nodo_master;
    int nodos_con_consultas[NUM_MAX_NODOS];

    sem_t sem_consultas_dentro, sem_nodo_master, sem_testigos_recogidos, sem_id_nodo_master, sem_nodos_con_consultas;

    //VARIABLES QUE USA CUALQUIER PROCESO DEL NODO:
    int testigo;
    int dentro;
    int turno_ANUL, turno_PAG_ADM, turno_RES, turno_CONS, turno;
    int atendidas[NUM_MAX_NODOS][P], peticiones[NUM_MAX_NODOS][P];
    int mi_peticion;
    int buzones_nodos[NUM_MAX_NODOS];
    int prioridad_maxima, prioridad_maxima_otro_nodo;

    sem_t sem_testigo, sem_dentro, sem_turno_ANUL, sem_turno_PAG_ADM, sem_turno_RES, sem_turno_CONS, sem_turno,
            sem_atendidas, sem_peticiones, sem_mi_peticion, sem_buzones_nodos, sem_prioridad_maxima,sem_prioridad_maxima_otro_nodo, ;

    //VARIABLES QUE USAN LOS ESCRITORES:
    int contador_anul_pendientes, contador_pag_adm_pendientes, contador_res_pendientes, contador_cons_pendientes;

    sem_t sem_contador_procesos_max_SC;
    sem_t sem_contador_anul_pendientes, sem_contador_pag_adm_pendientes, sem_contador_res_pendientes, sem_contador_cons_pendientes;
    sem_t sem_anul_pend, sem_pag_adm_pend, sem_res_pend, sem_cons_pend;
}memoria_nodo;

int max(int a, int b){
    return (a > b) ? a : b;
}

void calcular_prioridad_maxima(memoria_nodo *me){
    sem_wait(&(me->sem_contador_anul_pendientes));
    if(me->contador_anul_pendientes > 0){
        sem_post(&(me->sem_contador_anul_pendientes));
        sem_wait(&(me->sem_prioridad_maxima));
        me->prioridad_maxima = ANUL;
        sem_post(&(me->sem_prioridad_maxima));
    }else{
        sem_post(&(me->sem_contador_anul_pendientes));
        sem_wait(&(me->sem_contador_pag_adm_pendientes));
        if(me->contador_pag_adm_pendientes > 0){
            sem_post(&(me->sem_contador_pag_adm_pendientes));
            sem_wait(&(me->sem_prioridad_maxima));
            me->prioridad_maxima = PAG_ADM;
            sem_post(&(me->sem_prioridad_maxima));
        }else{
            sem_post(&(me->sem_contador_pag_adm_pendientes));
            sem_wait(&(me->sem_contador_res_pendientes));
            if(me->contador_res_pendientes > 0){
                sem_post(&(me->sem_contador_res_pendientes));
                sem_wait(&(me->sem_prioridad_maxima));
                me->prioridad_maxima = RESERVA;
                sem_post(&(me->sem_prioridad_maxima));
            }else{
                sem_post(&(me->sem_contador_res_pendientes));
                sem_wait(&(me->sem_contador_cons_pendientes));
                if(me->contador_cons_pendientes>0){
                    sem_post(&(me->sem_contador_cons_pendientes));
                    sem_wait(&(me->sem_prioridad_maxima));
                    me->prioridad_maxima = CONSULTA;
                    sem_post(&(me->sem_prioridad_maxima));
                }else{
                    sem_post(&(me->sem_contador_cons_pendientes));
                    sem_wait(&(me->sem_prioridad_maxima));
                    me->prioridad_maxima = 0;   // NO HAY NADA
                    sem_post(&(me->sem_prioridad_maxima));
                }
            }
        }
    }

    sem_wait(&(me->sem_prioridad_max_otro_nodo));
    sem_wait(&(me->sem_prioridad_maxima));
    #ifdef __DEBUG
        printf("\tDEBUG: Prioridad máxima en mi nodo: %d. En otro nodo: %d.\n",me->prioridad_maxima, me->prioridad_max_otro_nodo);
    #endif
    sem_post(&(me->sem_prioridad_max_otro_nodo));
    sem_post(&(me->sem_prioridad_maxima));
}




#endif