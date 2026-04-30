#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/shm.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <shmid1> <shmid2>\n", argv[0]);
        exit(1);
    }

    int shmid1 = atoi(argv[1]);
    int shmid2 = atoi(argv[2]);

    if (shmid1 > shmid2) {
        printf("Error: shmid1 debe ser <= shmid2\n");
        exit(1);
    }

    for (int shmid = shmid1; shmid <= shmid2; shmid++) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            printf("No se pudo eliminar shmid %d (errno=%d)\n", shmid, errno);
        } else {
            printf("Segmento %d eliminado\n", shmid);
        }
    }

    return 0;
}