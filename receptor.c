#include "procesos.h"

int main(int argc, char *argv[]){
    if(argc != 4){
        printf("La forma de ejecución correcta es %s [id nodo] [numNodos] [tiempoSC]\n", argv[0]);
        return EXIT_FAILURE;
    }
    //INICIALIZACIÓN DE LA MEMORIA COMPARTIDA, BUZONES Y SEMÁFOROS.
    int mi_id = atoi(argv[1]);
    int num_nodos = atoi(argv[2]);
    int tiempoSC = atoi(argv[3]);
    memoria_nodo *mem;
    int memoria_id = shmget(mi_id,sizeof(memoria_nodo), 0666 | IPC_CREAT);
    if(memoria_id == -1){
        perror("Error al crear la memoria compartida");
        return EXIT_FAILURE;
    }
    mem = shmat(memoria_id, NULL, 0);
    if(mem == (void *) -1){
        perror("Error al adjuntar la memoria compartida");
        return EXIT_FAILURE;
    }
    #ifdef __DEBUG
    printf("DEBUG: Memoria compartida creada y adjuntada correctamente. ID: %d\n", memoria_id);
    #endif
    if(mi_id == 1){
        #ifdef PRINT_RX
        printf("Soy el nodo 1 \n");
        #endif
        mem->testigo = 1; // El nodo 1 empieza con el testigo maestro
        mem->nodo_master = 1; // El nodo 1 es el nodo maestro
    }else{
        mem->testigo = 0;
        mem->nodo_master = 0;
    }

    mem->num_nodos = num_nodos;
    mem->tiempo_SC = tiempoSC;
    memset(mem->atendidas, 0, sizeof(mem->atendidas));
    memset(mem->peticiones, 0, sizeof(mem->peticiones));

    mem->testigos_recogidos = 0;
    mem->id_nodo_master = 0;
    mem->consultas_dentro = 0;
    mem->dentro = 0;
    mem-> mi_peticion = 0;
    mem->contador_anul_pendientes = 0;
    mem->contador_pag_adm_pendientes = 0;
    mem->contador_res_pendientes = 0;
    mem->contador_cons_pendientes = 0;
    mem->contador_procesos_max_SC = 0;
    mem->prioridad_maxima = 0;
    mem->prioridad_maxima_otro_nodo = 0;
    mem-> turno = 0;
    mem-> turno_ANUL = 0;
    mem-> turno_PAG_ADM = 0;
    mem-> turno_RES = 0;
    mem-> turno_CONS = 0;

    //inicialización de semáforos:

    //de sincronización:
    sem_init(&(mem->sem_anul_pend), 1, 0);
    sem_init(&(mem->sem_pag_adm_pend), 1, 0);
    sem_init(&(mem->sem_res_pend), 1, 0);
    sem_init(&(mem->sem_cons_pend), 1, 0);

    //de exclusión mutua:
    sem_init(&(mem->sem_contador_anul_pendientes), 1, 1);
    sem_init(&(mem->sem_contador_pag_adm_pendientes), 1, 1);
    sem_init(&(mem->sem_contador_res_pendientes), 1, 1);
    sem_init(&(mem->sem_contador_cons_pendientes), 1, 1);
    sem_init(&(mem->sem_mi_peticion), 1, 1);
    sem_init(&(mem->sem_prioridad_maxima), 1, 1);
    sem_init(&(mem->sem_prioridad_maxima_otro_nodo), 1, 1);
    sem_init(&(mem->sem_testigo), 1, 1);
    sem_init(&(mem->sem_contador_procesos_max_SC), 1, 1);
    sem_init(&(mem->sem_dentro), 1, 1);
    sem_init(&(mem->sem_atendidas), 1, 1);
    sem_init(&(mem->sem_peticiones), 1, 1);
    sem_init(&(mem->sem_turno), 1, 1);
    sem_init(&(mem->sem_turno_ANUL), 1, 1);
    sem_init(&(mem->sem_turno_PAG_ADM), 1, 1);
    sem_init(&(mem->sem_turno_RES), 1, 1);
    sem_init(&(mem->sem_turno_CONS), 1, 1);
    sem_init(&(mem->sem_buzones_nodos), 1, 1);

    //usadas por los lectores:
    sem_init(&(mem->sem_consultas_dentro), 1, 1);
    sem_init(&(mem->sem_nodo_master), 1, 1);
    sem_init(&(mem->sem_testigos_recogidos), 1, 1);
    sem_init(&(mem->sem_id_nodo_master), 1, 1);
    sem_init(&(mem->sem_nodos_con_consultas), 1, 1);

    for(int i=0; i< num_nodos; i++){
        mem->buzones_nodos[i] = msgget(i+1,IPC_CREAT | 0777);
        if(mem->buzones_nodos[i] == -1){
            perror("Error al crear el buzón del nodo");
            return EXIT_FAILURE;
        }
        #ifdef __DEBUG
        printf("DEBUG: Buzón creado para nodo %d con ID %d\n", i+1, mem->buzones_nodos[i]);
        #endif
    }
    //HASTA AQUÍ LA INICIALIZACIÓN DE LAS VARIABLES USADAS POR TODOS LOS PROCESOS DEL NODO.

    msgbuf_mensaje mensaje_rx;
    sem_wait(&(mem->sem_buzones_nodos));
    int mi_id_buzon = mem->buzones_nodos[mi_id-1];
    sem_post(&(mem->sem_buzones_nodos));

    while(1){
        if(msgrcv(mi_id_buzon, &mensaje_rx, sizeof(mensaje_rx)-sizeof(long), 0, 0) == -1){
            perror("Error al recibir mensaje en el receptor");
            return EXIT_FAILURE;
        }
        switch(mensaje_rx.msg_type){
            case 1: // SE RECIBE UNA PETICION DEL TESTIGO
            /* 
            1. Actualiza peticiones[][].
            2. Actualiza prioridad_maxima_otro_nodo.
            3. Si la petición es CONSULTA:
            - solo se manda copias si no hay prioridades superiores.
            4. Si la petición es de escritor:
            - si tiene el testigo y no está en consultas, puede enviar el testigo.
            - si está en consultas, cierras turno_CONS para que no entren más consultas nuevas.
            */


            #ifdef PRINT_RX
            printf("[RECEPTOR]Nodo %d ha recibido una petición (%d) del nodo %d con prioridad %d\n",mi_id, mensaje_rx.peticion, mensaje_rx.id, mensaje_rx.prioridad);
            #endif
            sem_wait(&(mem->sem_peticiones));
            mem->peticiones[mensaje_rx.id-1][mensaje_rx.prioridad-1] = max(mem->peticiones[mensaje_rx.id-1][mensaje_rx.prioridad-1], mensaje_rx.peticion);
            sem_post(&(mem->sem_peticiones));
            sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
            mem->prioridad_maxima_otro_nodo = max(mem->prioridad_maxima_otro_nodo, mensaje_rx.prioridad);
            sem_post(&(mem->sem_prioridad_maxima_otro_nodo));

            if(mensaje_rx.prioridad == CONSULTA){
                sem_wait(&mem->sem_prioridad_maxima);
                sem_wait(&mem->sem_prioridad_maxima_otro_nodo);
                sem_wait(&mem->sem_testigo);

                if(mem->testigo == 1 && mem->prioridad_maxima_otro_nodo == CONSULTA && (mem->prioridad_maxima == CONSULTA || mem->prioridad_maxima == 0)) {
                    sem_post(&mem->sem_testigo);
                    sem_post(&mem->sem_prioridad_maxima_otro_nodo);
                    sem_post(&mem->sem_prioridad_maxima);
                    send_testigo_falso(mi_id,mem);
                }else{
                    sem_post(&mem->sem_testigo);
                    sem_post(&mem->sem_prioridad_maxima_otro_nodo);
                    sem_post(&mem->sem_prioridad_maxima);
                }
            } else{
                sem_wait(&(mem->sem_testigo));
                sem_wait(&(mem->sem_turno_CONS));
                if(mem->testigo == 1 && mem->turno_CONS == 0){
                    sem_post(&(mem->sem_turno_CONS));
                    sem_post(&(mem->sem_testigo));
                    sem_wait(&(mem->sem_prioridad_maxima));
                    if(mem->prioridad_maxima == 0 || mensaje_rx.prioridad > mem->prioridad_maxima){
                        sem_post(&(mem->sem_prioridad_maxima));
                        send_testigo(mi_id, mem);
                    }else{
                        sem_post(&(mem->sem_prioridad_maxima));
                    }
                }else{ 
                    sem_post(&(mem->sem_turno_CONS));
                    sem_post(&(mem->sem_testigo));

                    if(mensaje_rx.prioridad > CONSULTA){
                        sem_wait(&(mem->sem_turno_CONS));
                        mem->turno_CONS = 0;    //Se le cierra el grifo a los lectores
                        sem_post(&(mem->sem_turno_CONS));
                    }
                }
            }


            break;
            case 2: // SE RECIBE EL TESTIGO MAESTRO
            break;
            case 3: // SE RECIBE UN TESTIGO COPIA PARA DAR ACCESO A LEER 
            break;
            case 4: // SE RECIBE UN TESTIGO COPIA UNA VEZ QUE SE HA TERMINADO DE LEER
            break;
        }
    }


}