# PCCD-Grupo-C
## Miembros:
+ Marcos Sanjurjo.
+ Marco Merino.
+ Nicolas Pérez.
+ Andrés Rouco.

## Uso:
Para ejecutar los escenarios (ya sea uno de los ficheros de `.config` o `.txt` de /sims) ejecutar antes el script que limpia, compila y prepara el entorno de ejecucion previo:

```Bash
sudo ./Scripts/pianoPiano.sh
```
(sudo es necesario por borrar todos los ipcs)

### inicio.c 
+ El inicio por defecto. 
    + El argv[2] sirve para activar la llegada de pagos en medio de otros procesos. 
```Bash
sudo ./

# Por ejemplo:
sudo ./Scripts/inicio sims/sim3.txt 0 # Comportamiento por defecto.
# ó
sudo ./Scripts/inicio sims/sim3.txt 0 # Comienzan a llegar pagos a la mitad de la ejecución. (Requiere descomentar el código según interés).
```

### inicio_solo_pagos.c
+ Para tomar medidas para las simulaciones de solo PAGOS.

```Bash
sudo ./Scripts/inicio_solo_pagos <fichero_config_con_solo_pagos>

# Por ejemplo:
sudo ./Scripts/inicio_solo_pagos sims/sim3.txt
```

### inicio_rafagas.c 
+ Para ejecutar simulaciones con ráfagas (solo pagos).
```Bash
sudo ./Scripts/inicio_rafagas <fichero_config_solo_pagos_con_ráfagas>
# Ejemplo
sudo ./Scripts/inicio_rafagas prueba3.config
```
