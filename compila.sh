gcc -Wall consultas.c -o consulta -lpthread -D __DEBUG
gcc -Wall administraciones.c -o administraciones -lpthread -D __DEBUG
gcc -Wall reservas.c -o reservas -lpthread -D __DEBUG
gcc -Wall pagos.c -o pagos -lpthread -D __DEBUG
gcc -Wall receptor.c -o receptor -lpthread -D __DEBUG
gcc -Wall anulaciones.c -o anulaciones -lpthread -D __DEBUG
gcc -Wall inicio.c -o inicio -lpthread -D __DEBUG 

echo "PROGRAMAS COMPILADOS CORRECTAMENTE"

exit