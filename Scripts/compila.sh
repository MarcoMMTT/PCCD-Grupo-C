gcc -Wall consultas.c -o consultas -lpthread
gcc -Wall administraciones.c -o administraciones -lpthread
gcc -Wall reservas.c -o reservas -lpthread
gcc -Wall pagos.c -o pagos -lpthread
gcc -Wall receptor.c -o receptor -lpthread
gcc -Wall anulaciones.c -o anulaciones -lpthread
gcc -Wall inicio.c -o inicio -lpthread
gcc -Wall inicio_solo_pagos.c -o inicio_solo_pagos -lpthread
gcc -Wall inicio_rafagas.c -o inicio_rafagas -lpthread

echo "PROGRAMAS COMPILADOS CORRECTAMENTE"

exit