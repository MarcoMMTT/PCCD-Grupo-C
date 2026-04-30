#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Se necesitan dos argumentos: el primero el primer id de buzón, el segundo el último id de buzón a borrar\n");
        printf("Uso: %s [primer_id] [ultimo_id]\n", argv[0]);
        return 1;
    }

    int primero = atoi(argv[1]);
    int ultimo = atoi(argv[2]);

    if (primero >= ultimo) {
        printf("El primer argumento debe ser menor que el segundo\n");
        return 1;
    }

    for (int i = primero; i <= ultimo; i++) {
        if (msgctl(i, IPC_RMID, NULL) == -1) {
            printf("Error al eliminar la cola %d: %s\n", i, strerror(errno));
        } else {
            printf("Cola de mensajes %d eliminada\n", i);
        }
    }

    printf("Buzones borrados\n");

    return 0;
}