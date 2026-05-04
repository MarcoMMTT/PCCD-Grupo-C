gcc -Wall consultas.c -o consultas -lpthread
gcc -Wall administraciones.c -o administraciones -lpthread
gcc -Wall reservas_git.c -o reservas -lpthread
gcc -Wall pagos.c -o pagos -lpthread
gcc -Wall receptor.c -o receptor -lpthread
gcc -Wall anulaciones.c -o anulaciones -lpthread
gcc -Wall inicio.c -o inicio -lpthread
gcc -Wall eliminarTodo.c -o eliminarTodo -lpthread
# gcc -Wall eliminarShm.c -o eliminarShm -lpthread

echo "PROGRAMAS COMPILADOS CORRECTAMENTE"

exit