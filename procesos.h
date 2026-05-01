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
 * @brief Estructura para los mensajes del sistema de comunicación distribuida.
 *
 * Esta estructura encapsula los mensajes que se intercambian entre nodos en el sistema.
 * Define el tipo de mensaje, la identidad del nodo, información de peticiones y prioridades,
 * así como la matriz de peticiones atendidas y la identificación del nodo maestro.
 *
 * @param msg_type Tipo de mensaje (1=SOLICITUD, 2=TESTIGO_MAESTRO, 3=ENVIAR_TESTIGO_COPIA, 4=RECIBIR_TESTIGO_COPIA)
 * @param id ID del nodo remitente
 * @param peticion Número secuencial de la petición del nodo
 * @param prioridad Nivel de prioridad de la petición (1=CONSULTA, 2=RESERVA, 3=PAG_ADM, 4=ANUL)
 * @param atendidas[NUM_MAX_NODOS][P] Matriz que registra las peticiones ya atendidas de cada nodo por tipo de prioridad
 * @param id_nodo_master ID del nodo que actualmente posee el testigo maestro
 */
typedef struct {
  long msg_type;
  int id;
  int peticion;
  int prioridad;
  int atendidas[NUM_MAX_NODOS][P];
  int id_nodo_master;
}msgbuf_mensaje;


/**
 * @brief Estructura de memoria compartida del nodo.
 *
 * Esta estructura reside en memoria compartida y mantiene el estado global del nodo.
 * Gestiona variables de sincronización, turnos de acceso a secciones críticas, contadores
 * de peticiones pendientes y semáforos para coordinar múltiples procesos dentro del nodo.
 *
 * VARIABLES DE CONFIGURACIÓN (leídas del fichero config):
 *   - num_nodos: Número total de nodos en el sistema distribuido
 *   - tiempo_SC: Duración máxima permitida en sección crítica
 *
 * VARIABLES PARA LECTORES (Procesos de Consulta):
 *   - consultas_dentro: Contador de procesos de consulta en sección crítica
 *   - id_nodo_master: ID del nodo que posee el testigo maestro
 *   - testigos_recogidos: Cantidad de testigos recolectados
 *   - nodos_con_consultas[NUM_MAX_NODOS]: Registro de qué nodos tienen consultas pendientes
 *   - sem_*: Semáforos para proteger acceso a estas variables
 *
 * VARIABLES DE CONTROL DEL PROTOCOLO (para todos los procesos):
 *   - testigo: Indicador de posesión del testigo (0=no tiene, 1=sí tiene)
 *   - dentro: Indicador si el proceso está en sección crítica
 *   - turno_*: Contadores de turno para cada tipo de petición (ANUL, PAG_ADM, RES, CONS)
 *   - atendidas[NUM_MAX_NODOS][P]: Matriz de peticiones ya atendidas por nodo y prioridad
 *   - peticiones[NUM_MAX_NODOS][P]: Matriz de peticiones pendientes por nodo y prioridad
 *   - mi_peticion: Número de petición actual del proceso
 *   - buzones_nodos[NUM_MAX_NODOS]: IDs de buzones de mensaje de cada nodo
 *   - prioridad_maxima: Prioridad máxima en este nodo
 *   - prioridad_maxima_otro_nodo: Prioridad máxima en otros nodos (para comparación)
 *   - sem_*: Semáforos para exclusión mutua
 *
 * VARIABLES PARA ESCRITORES (Procesos generadores de peticiones):
 *   - contador_*_pendientes: Contadores de peticiones pendientes por tipo
 *   - sem_contador_*: Semáforos para proteger los contadores
 *   - sem_*_pend: Semáforos adicionales para sincronización de pendientes
 */
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
            sem_atendidas, sem_peticiones, sem_mi_peticion, sem_buzones_nodos, sem_prioridad_maxima,sem_prioridad_maxima_otro_nodo;

    //VARIABLES QUE USAN LOS ESCRITORES:
    int contador_anul_pendientes, contador_pag_adm_pendientes, contador_res_pendientes, contador_cons_pendientes;

    sem_t sem_contador_procesos_max_SC;
    sem_t sem_contador_anul_pendientes, sem_contador_pag_adm_pendientes, sem_contador_res_pendientes, sem_contador_cons_pendientes;
    sem_t sem_anul_pend, sem_pag_adm_pend, sem_res_pend, sem_cons_pend;
}memoria_nodo;

/**
 * @brief Retorna el máximo entre dos números enteros.
 *
 * Función de utilidad que compara dos valores enteros y devuelve el mayor.
 * Utilizada en contextos donde se necesita seleccionar el valor máximo entre dos opciones.
 *
 * @param a Primer valor entero a comparar
 * @param b Segundo valor entero a comparar
 * @return El valor mayor entre a y b
 */
int max(int a, int b){
    return (a > b) ? a : b;
}

/**
 * @brief Calcula y actualiza la prioridad máxima de peticiones pendientes en el nodo.
 *
 * Esta función implementa un algoritmo jerárquico para determinar la prioridad más alta
 * entre todas las peticiones pendientes en el nodo actual. El algoritmo evalúa, en orden
 * de prioridad descendente, cuántas peticiones de cada tipo hay pendientes:
 *   1. ANUL (4) - Anulaciones
 *   2. PAG_ADM (3) - Pagos y Administración
 *   3. RESERVA (2) - Reservas
 *   4. CONSULTA (1) - Consultas
 *
 * Si se encuentran peticiones de una categoría, se asigna esa prioridad como la máxima.
 * Si no hay peticiones en ninguna categoría, la prioridad se establece en 0.
 *
 * La función utiliza semáforos para garantizar acceso exclusivo a:
 *   - Los contadores de peticiones pendientes (sem_contador_*_pendientes)
 *   - La variable que almacena la prioridad máxima (sem_prioridad_maxima)
 *   - La información de prioridades en otros nodos (sem_prioridad_max_otro_nodo)
 *
 * Operación:
 *   1. Adquiere semáforos para acceso exclusivo a contadores
 *   2. Evalúa contadores en orden: ANUL → PAG_ADM → RESERVA → CONSULTA
 *   3. Asigna la prioridad máxima al primer contador > 0 encontrado
 *   4. Libera semáforos tras actualización
 *   5. Si DEBUG está definido, imprime información de diagnóstico
 *
 * @param mem Puntero a la estructura de memoria compartida del nodo
 *
 * @note Esta función es crítica en el protocolo de exclusión mutua distribuida,
 *       permitiendo que el nodo determine qué tipo de petición procesar primero.
 */
void calcular_prioridad_maxima(memoria_nodo *mem){
    sem_wait(&(mem->sem_contador_anul_pendientes));
    if(mem->contador_anul_pendientes > 0){
        sem_post(&(mem->sem_contador_anul_pendientes));
        sem_wait(&(mem->sem_prioridad_maxima));
        mem->prioridad_maxima = ANUL;
        sem_post(&(mem->sem_prioridad_maxima));
    }else{
        sem_post(&(mem->sem_contador_anul_pendientes));
        sem_wait(&(mem->sem_contador_pag_adm_pendientes));
        if(mem->contador_pag_adm_pendientes > 0){
            sem_post(&(mem->sem_contador_pag_adm_pendientes));
            sem_wait(&(mem->sem_prioridad_maxima));
            mem->prioridad_maxima = PAG_ADM;
            sem_post(&(mem->sem_prioridad_maxima));
        }else{
            sem_post(&(mem->sem_contador_pag_adm_pendientes));
            sem_wait(&(mem->sem_contador_res_pendientes));
            if(mem->contador_res_pendientes > 0){
                sem_post(&(mem->sem_contador_res_pendientes));
                sem_wait(&(mem->sem_prioridad_maxima));
                mem->prioridad_maxima = RESERVA;
                sem_post(&(mem->sem_prioridad_maxima));
            }else{
                sem_post(&(mem->sem_contador_res_pendientes));
                sem_wait(&(mem->sem_contador_cons_pendientes));
                if(mem->contador_cons_pendientes>0){
                    sem_post(&(mem->sem_contador_cons_pendientes));
                    sem_wait(&(mem->sem_prioridad_maxima));
                    mem->prioridad_maxima = CONSULTA;
                    sem_post(&(mem->sem_prioridad_maxima));
                }else{
                    sem_post(&(mem->sem_contador_cons_pendientes));
                    sem_wait(&(mem->sem_prioridad_maxima));
                    mem->prioridad_maxima = 0;   // NO HAY NADA
                    sem_post(&(mem->sem_prioridad_maxima));
                }
            }
        }
    }

    sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
    sem_wait(&(mem->sem_prioridad_maxima));
    #ifdef __DEBUG
        printf("\tDEBUG: Prioridad máxima en mi nodo: %d. En otro nodo: %d.\n",me->prioridad_maxima, me->prioridad_max_otro_nodo);
    #endif
    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
    sem_post(&(mem->sem_prioridad_maxima));
}

void send_testigo(int mi_id, memoria_nodo *mem){
    int i=0, j=0;
    int id_buscar;
    int id_inicio;
    int encontrado = 0;

    struct msgbuf_mensaje mensaje_testigo;
    mensaje_testigo.msg_type = (long)2; // TESTIGO_MAESTRO
    mensaje_testigo.id = mi_id;

    if(mi_id + 1 > mem->num_nodos){
        id_buscar = 1;
    }else{
        id_buscar = mi_id + 1;
    }
    id_inicio = id_buscar;

    // Reiniciamos el contador de retención del testigo
    sem_wait(&(mem->sem_contador_procesos_max_SC));
    mem->contador_procesos_max_SC = 0;
    sem_post(&(mem->sem_contador_procesos_max_SC));

    /*
     * Si no quedan procesos pendientes en el nodo se actualiza 
     * la petición de esa prioridad como atendida
    */

    sem_wait(&(mem->sem_atendidas));
    sem_wait(&(mem->sem_peticiones));

    sem_wait(&(mem->sem_contador_anul_pendientes));
    if(mem->contador_anul_pendientes == 0){
        mem->atendidas[mi_id-1][ANUL-1] = mem->peticiones[mi_id-1][ANUL-1];
    }
    sem_post(&(mem->sem_contador_anul_pendientes));

    sem_wait(&(mem->sem_contador_pag_adm_pendientes));
    if(mem->contador_pag_adm_pendientes == 0){
        mem->atendidas[mi_id-1][PAG_ADM-1] = mem->peticiones[mi_id-1][PAG_ADM-1];
    }
    sem_post(&(mem->sem_contador_pag_adm_pendientes));

    sem_wait(&(mem->sem_contador_res_pendientes));
    if(mem->contador_res_pendientes == 0){
        mem->atendidas[mi_id-1][RESERVA-1] = mem->peticiones[mi_id-1][RESERVA-1];
    }
    sem_post(&(mem->sem_contador_res_pendientes));

    sem_wait(&(mem->sem_contador_cons_pendientes));
    if(mem->contador_cons_pendientes == 0){
        mem->atendidas[mi_id-1][CONSULTA-1] = mem->peticiones[mi_id-1][CONSULTA-1];
    }
    sem_post(&(mem->sem_contador_cons_pendientes));

    sem_post(&(mem->sem_atendidas));
    sem_post(&(mem->sem_peticiones));

    /*
     * Buscar destinatario del testigo
     * Se recorren las prio de mayor a menor.
    */

    for(j= P -1; j>=0; j--){
        id_buscar = id_inicio;
        for(i=0; i < mem->num_nodos; i++){
            if(id_buscar > mem->num_nodos){
                id_buscar = 1;
            }
            if(id_buscar != mi_id){
                sem_wait(&(mem->sem_peticiones));
                sem_wait(&(mem->sem_atendidas));
                
                if(mem->peticiones[id_buscar-1][j] > mem->atendidas[id_buscar-1][j]){
                    sem_post(&(mem->sem_atendidas));
                    sem_post(&(mem->sem_peticiones));
                    encontrado = 1;

                    sem_wait(&(mem->sem_nodo_master));
                    mem-> nodo_master = 0; // Ya no es nodo maestro
                    sem_post(&(mem->sem_nodo_master));
                    break;
                }
                sem_post(&(mem->sem_atendidas));
                sem_post(&(mem->sem_peticiones));
            }
            id_buscar++;
        }
        if(encontrado){
            break;
        }

    }

    if(encontrado){
        mensaje_testigo.id = id_buscar;
        sem_wait(&(mem->sem_atendidas));
        memcpy(mensaje_testigo.atendidas, mem->atendidas, sizeof(mem->atendidas));
        sem_post(&(mem->sem_atendidas));

        sem_wait(&(mem->sem_testigo));
        mem->testigo = false;
        sem_post(&(mem->sem_testigo));

        sem_wait(&(mem->sem_buzones_nodos));
        if(msgsnd(
            mem->buzones_nodos[id_buscar-1], &mensaje_testigo, sizeof(msgbuf_mensaje) - sizeof(long), 0) == -1){
            perror("Error al enviar el testigo");
        }
        sem_post(&(mem->sem_buzones_nodos));
                #ifdef __DEBUG
        printf("Testigo enviado desde nodo %d al nodo %d\n", mi_id, id_buscar);
        #endif

    } else {
        #ifdef __DEBUG
        printf("DEBUG: No hay ningún nodo esperando el testigo.\n");
        #endif
    }

}

#endif