/**
 * @file reservas.c
 * @brief Proceso de gestión de reservas (Escritor - Prioridad 2)
 * Implementa Algoritmo de Ricart-Agrawala con paso de testigo usando procesos.h
 */

#include "procesos.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <id_nodo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mi_id = atoi(argv[1]);
    // Vincular la memoria compartida
    int memoria_id = shmget(mi_id, sizeof(memoria_nodo), 0);
    if (memoria_id == -1) {
        perror("Error: No se pudo conectar a la memoria compartida.");
        return EXIT_FAILURE;
    }
    memoria_nodo *mem = (memoria_nodo *)shmat(memoria_id, NULL, 0);
    
    // Incrementar el contador de procesos de reserva pendientes en el nodo
    sem_wait(&(mem->sem_contador_res_pendientes));
    mem->contador_res_pendientes++;
    sem_post(&(mem->sem_contador_res_pendientes));

    if (mem->contador_res_pendientes == 1) { // Posible Inanición si se contador_res_pendientes++ por segunda vez antes del if 
        // Tenemos el testigo en el nodo y nadie está en la Sección Crítica
        if (!(mem->testigo) && mem->dentro == 0) {
            calcular_prioridad_maxima(mem);
            send_peticiones(mi_id, mem, RESERVA); // Ya hace petición++
            sem_wait(&(mem->sem_res_pend));
            mem->testigo = 1;
            
        }
    }


    // Evaluar si tenemos el testigo y si el nodo está libre
    sem_wait(&(mem->sem_testigo));
    sem_wait(&(mem->sem_dentro));
    // Tenemos el testigo en el nodo y nadie está en la Sección Crítica
    if (mem->testigo && mem->dentro) {
        
        sem_wait(&(mem->sem_turno_RES));
        mem->turno_RES = 1;
        sem_post(&(mem->sem_turno_RES));
        
        sem_wait(&(mem->sem_turno));
        mem->turno = 1;
        sem_post(&(mem->sem_turno));
        
        // Despertamos a nuestro propio proceso para que acceda
        sem_post(&(mem->sem_res_pend));
    } else if (mem->testigo == 0) {
        // No tenemos el testigo, hacemos broadcast de solicitud (SEND REQUEST)
        // send_peticiones incrementa mi_peticion internamente y envía a buzones
        send_peticiones(mi_id, mem, RESERVA);
    }
    
    sem_post(&(mem->sem_dentro));
    sem_post(&(mem->sem_testigo));

    // -------------------------------------------------------------------------
    // ESPERA Y ENTRADA A SECCIÓN CRÍTICA
    // -------------------------------------------------------------------------
    
    // WAIT(sem_proceso_esp) o esperar recepción de testigo (RECEIVE TOKEN)
    // Nos bloqueamos hasta que el proceso RX o el proceso saliente nos despierte
    sem_wait(&(mem->sem_res_pend));

    // Marcar que estamos DENTRO de la sección crítica
    sem_wait(&(mem->sem_dentro));
    mem->dentro = 1;
    sem_post(&(mem->sem_dentro));

    // --- INICIO SECCIÓN CRÍTICA ---
    printf("[NODO %d] RESERVAS: Entrando en la Sección Crítica...\n", mi_id);
    sleep(mem->tiempo_SC); // Simular el tiempo de operación de la reserva
    printf("[NODO %d] RESERVAS: Saliendo de la Sección Crítica.\n", mi_id);
    // --- FIN SECCIÓN CRÍTICA ---

    // -------------------------------------------------------------------------
    // FASE DE LIBERACIÓN Y ACTUALIZACIÓN (Adaptación de vector_atendidas)
    // -------------------------------------------------------------------------

    // Decrementar peticiones pendientes
    sem_wait(&(mem->sem_contador_res_pendientes));
    mem->contador_res_pendientes--;
    sem_post(&(mem->sem_contador_res_pendientes));

    // Actualizar matriz de peticiones atendidas para este nodo y prioridad
    sem_wait(&(mem->sem_atendidas));
    sem_wait(&(mem->sem_peticiones));
    mem->atendidas[mi_id-1][RESERVA-1] = mem->peticiones[mi_id-1][RESERVA-1];
    sem_post(&(mem->sem_peticiones));
    sem_post(&(mem->sem_atendidas));

    // Salir de la sección crítica localmente
    sem_wait(&(mem->sem_dentro));
    mem->dentro = 0;
    sem_post(&(mem->sem_dentro));

    sem_wait(&(mem->sem_turno_RES));
    mem->turno_RES = 0;
    sem_post(&(mem->sem_turno_RES));

    sem_wait(&(mem->sem_turno));
    mem->turno = 0;
    sem_post(&(mem->sem_turno));

    // -------------------------------------------------------------------------
    // TOMA DE DECISIÓN: CEDER O RETENER EL TESTIGO
    // -------------------------------------------------------------------------
    
    // Comprobar retención de testigo (para evitar inanición en el sistema)
    sem_wait(&(mem->sem_contador_procesos_max_SC));
    mem->contador_procesos_max_SC++;
    int accesos_consecutivos = mem->contador_procesos_max_SC;
    sem_post(&(mem->sem_contador_procesos_max_SC));

    // Calcular qué proceso tiene la mayor prioridad en el nodo local
    calcular_prioridad_maxima(mem);

    sem_wait(&(mem->sem_prioridad_maxima));
    int prio_max = mem->prioridad_maxima;
    sem_post(&(mem->sem_prioridad_maxima));

    // Lógica para despertar un proceso local o enviar el testigo a otro nodo
    if (prio_max > 0 && accesos_consecutivos < EVITAR_RETENCION) {
        // Hay procesos locales esperando y no hemos excedido el límite de retención
        if (prio_max == ANUL) sem_post(&(mem->sem_anul_pend));
        else if (prio_max == PAG_ADM) sem_post(&(mem->sem_pag_adm_pend));
        else if (prio_max == RESERVA) sem_post(&(mem->sem_res_pend));
        else if (prio_max == CONSULTA) sem_post(&(mem->sem_cons_pend));
    } else {
        // Excedimos retención o no hay peticiones locales; buscamos siguiente nodo
        // send_testigo busca circularmente según prioridades y matriz de atendidas
        send_testigo(mi_id, mem);
    }

    // Desvincular la memoria compartida
    shmdt(mem);

    return 0;
}
