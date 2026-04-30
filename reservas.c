#include procesos.h
#include <sys/time.h> 
#include <stdlib.h>


int main (int argc, char *argv[]) {
    if (argc != 2){
    printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
    return EXIT_FAILURE;
    }
    
}