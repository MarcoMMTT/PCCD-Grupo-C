# SOLUCIONES - Correcciones Específicas por Archivo

## 📋 PATRON GENERAL DE CORRECCIÓN

### Problema: Múltiples sem_wait con liberación inconsistente

**Código INCORRECTO** (pagos.c líneas 35-80):
```c
sem_wait(&(mem->sem_contador_pag_adm_pendientes));     // 1
mem->contador_pag_adm_pendientes++;
sem_wait(&(mem->sem_testigo));                          // 2
sem_wait(&(mem->sem_turno_PAG_ADM));                    // 3
sem_wait(&(mem->sem_turno));                            // 4
sem_wait(&(mem->sem_contador_procesos_max_SC));         // 5

// ⚠️ Lee variables SIN protección
if ((!mem->testigo && mem->contador_pag_adm_pendientes == 1) ||
    (mem->testigo && mem->turno_PAG_ADM && ...)) {
    
    sem_post(&(mem->sem_testigo));        // Libera 2
    sem_post(&(mem->sem_turno_PAG_ADM));  // Libera 3
    // ... más posts ...
    
} else {
    sem_post(&(mem->sem_testigo));        // Libera 2
    // ... posts incompletos ...
}
// ❌ PROBLEMA: Algunos sem_wait nunca se libera en ciertos caminos
```

**Solución CORRECTA** - Patrón con cleanup garantizado:

```c
// 1. ADQUIRIR EN ORDEN FIJO
sem_wait(&(mem->sem_contador_pag_adm_pendientes));     // 1
mem->contador_pag_adm_pendientes++;
sem_wait(&(mem->sem_testigo));                          // 2
sem_wait(&(mem->sem_turno_PAG_ADM));                    // 3
sem_wait(&(mem->sem_turno));                            // 4
sem_wait(&(mem->sem_contador_procesos_max_SC));         // 5

// 2. LEER VARIABLES PROTEGIDAS (mientras tenemos los semáforos)
int testigo_local = mem->testigo;
int contador_local = mem->contador_pag_adm_pendientes;
int turno_pag_local = mem->turno_PAG_ADM;
int turno_local = mem->turno;
int procesos_max_local = mem->contador_procesos_max_SC;

// 3. LIBERAR EN ORDEN INVERSO ANTES DE LÓGICA
sem_post(&(mem->sem_contador_procesos_max_SC));        // Libera 5
sem_post(&(mem->sem_turno));                            // Libera 4
sem_post(&(mem->sem_turno_PAG_ADM));                    // Libera 3
sem_post(&(mem->sem_testigo));                          // Libera 2
sem_post(&(mem->sem_contador_pag_adm_pendientes));     // Libera 1

// 4. USAR VARIABLES LOCALES EN LÓGICA
if ((!testigo_local && contador_local == 1) ||
    (testigo_local && turno_pag_local && ...)) {
    
    // Pedir el testigo
    sem_wait(&(mem->sem_turno_CONS));
    mem->turno_CONS = 0;
    sem_post(&(mem->sem_turno_CONS));
    
    calcular_prioridad_maxima(mem);
    send_peticiones(mi_id, mem, PAG_ADM);
    
    sem_wait(&(mem->sem_pag_adm_pend));
    
} else {
    // No pedir el testigo
    // Lógica alternativa...
    sem_wait(&(mem->sem_pag_adm_pend));
}
```

**VENTAJAS de esta solución**:
- ✅ Semáforos SIEMPRE se liberan en orden inverso
- ✅ NO hay race conditions porque leemos con semáforos adquiridos
- ✅ NO hay deadlock porque no retenemos semáforos durante I/O
- ✅ Código más legible

---

## 🔧 CORRECCIONES ESPECÍFICAS POR ARCHIVO

### File: pagos.c

**Línea 35-45 - CORRECCIÓN**:

**ANTES:**
```c
sem_wait(&(mem->sem_contador_pag_adm_pendientes));
mem->contador_pag_adm_pendientes++;
sem_wait(&(mem->sem_testigo));
sem_wait(&(mem->sem_turno_PAG_ADM));
sem_wait(&(mem->sem_turno));
sem_wait(&(mem->sem_contador_procesos_max_SC));

if ((!mem->testigo && mem->contador_pag_adm_pendientes == 1) ||
```

**DESPUÉS:**
```c
sem_wait(&(mem->sem_contador_pag_adm_pendientes));
mem->contador_pag_adm_pendientes++;
sem_wait(&(mem->sem_testigo));
sem_wait(&(mem->sem_turno_PAG_ADM));
sem_wait(&(mem->sem_turno));
sem_wait(&(mem->sem_contador_procesos_max_SC));

// Copiar valores con semáforos tomados
int testigo_local = mem->testigo;
int contador_local = mem->contador_pag_adm_pendientes;
int turno_pag_local = mem->turno_PAG_ADM;
int turno_local = mem->turno;

// LIBERAR EN ORDEN INVERSO
sem_post(&(mem->sem_contador_procesos_max_SC));
sem_post(&(mem->sem_turno));
sem_post(&(mem->sem_turno_PAG_ADM));
sem_post(&(mem->sem_testigo));
sem_post(&(mem->sem_contador_pag_adm_pendientes));

// Ahora usar valores locales
if ((!testigo_local && contador_local == 1) ||
    (testigo_local && turno_pag_local && (contador_local + mem->contador_procesos_max_SC - EVITAR_RETENCION == 1)) ||
    (testigo_local && (contador_local == 1) && !turno_pag_local && turno_local)){
```

---

### File: receptor.c

**Línea 48-70 - CORRECCIÓN EN LECTURA DE VARIABLES**:

**ANTES:**
```c
if(mensaje_rx.prioridad == CONSULTA){
    sem_wait(&mem->sem_prioridad_maxima);
    sem_wait(&mem->sem_prioridad_maxima_otro_nodo);
    sem_wait(&mem->sem_testigo);

    if(mem->testigo == 1 && mem->prioridad_maxima_otro_nodo == CONSULTA && 
       (mem->prioridad_maxima == CONSULTA || mem->prioridad_maxima == 0)) {
        sem_post(&mem->sem_testigo);
        sem_post(&mem->sem_prioridad_maxima_otro_nodo);
        sem_post(&mem->sem_prioridad_maxima);
        send_testigo_copia(mi_id,mem);
```

**DESPUÉS:**
```c
if(mensaje_rx.prioridad == CONSULTA){
    sem_wait(&mem->sem_prioridad_maxima);
    sem_wait(&mem->sem_prioridad_maxima_otro_nodo);
    sem_wait(&mem->sem_testigo);

    // LEER VARIABLES MIENTRAS TENEMOS SEMÁFOROS
    int testigo_local = mem->testigo;
    int prioridad_maxima_otro_local = mem->prioridad_maxima_otro_nodo;
    int prioridad_maxima_local = mem->prioridad_maxima;
    
    // LIBERAR EN ORDEN INVERSO
    sem_post(&mem->sem_testigo);
    sem_post(&mem->sem_prioridad_maxima_otro_nodo);
    sem_post(&mem->sem_prioridad_maxima);

    // USAR VARIABLES LOCALES
    if(testigo_local == 1 && prioridad_maxima_otro_local == CONSULTA && 
       (prioridad_maxima_local == CONSULTA || prioridad_maxima_local == 0)) {
        send_testigo_copia(mi_id,mem);
```

---

### File: procesos.h - Función calcular_prioridad_maxima()

**Línea 184-240 - PROBLEMA DE VENTANA DE RACE CONDITION**

**ANTES (INCORRECTO):**
```c
void calcular_prioridad_maxima(memoria_nodo *mem){
    sem_wait(&(mem->sem_contador_anul_pendientes));
    if(mem->contador_anul_pendientes > 0){
        sem_post(&(mem->sem_contador_anul_pendientes));  // ⚠️ LIBERA
        // VENTANA AQUÍ: otro proceso puede cambiar el contador
        sem_wait(&(mem->sem_prioridad_maxima));
        mem->prioridad_maxima = ANUL;
        sem_post(&(mem->sem_prioridad_maxima));
```

**DESPUÉS (CORRECTO):**
```c
void calcular_prioridad_maxima(memoria_nodo *mem){
    int prioridad_maxima_local = 0;
    
    // Comprobar ANUL
    sem_wait(&(mem->sem_contador_anul_pendientes));
    if(mem->contador_anul_pendientes > 0){
        prioridad_maxima_local = ANUL;
    }
    sem_post(&(mem->sem_contador_anul_pendientes));
    
    // Comprobar PAG_ADM solo si no encontramos ANUL
    if(prioridad_maxima_local == 0) {
        sem_wait(&(mem->sem_contador_pag_adm_pendientes));
        if(mem->contador_pag_adm_pendientes > 0){
            prioridad_maxima_local = PAG_ADM;
        }
        sem_post(&(mem->sem_contador_pag_adm_pendientes));
    }
    
    // Comprobar RESERVA solo si no encontramos anteriores
    if(prioridad_maxima_local == 0) {
        sem_wait(&(mem->sem_contador_res_pendientes));
        if(mem->contador_res_pendientes > 0){
            prioridad_maxima_local = RESERVA;
        }
        sem_post(&(mem->sem_contador_res_pendientes));
    }
    
    // Comprobar CONSULTA solo si no encontramos anteriores
    if(prioridad_maxima_local == 0) {
        sem_wait(&(mem->sem_contador_cons_pendientes));
        if(mem->contador_cons_pendientes > 0){
            prioridad_maxima_local = CONSULTA;
        }
        sem_post(&(mem->sem_contador_cons_pendientes));
    }
    
    // AHORA actualizar la variable compartida de una sola vez
    sem_wait(&(mem->sem_prioridad_maxima));
    mem->prioridad_maxima = prioridad_maxima_local;
    sem_post(&(mem->sem_prioridad_maxima));

    // Actualizar otro_nodo sin ventanas de raza
    sem_wait(&(mem->sem_prioridad_maxima_otro_nodo));
    sem_wait(&(mem->sem_prioridad_maxima));
    #ifdef __DEBUG
        printf("\tDEBUG: Prioridad máxima en mi nodo: %d. En otro nodo: %d.\n",
               mem->prioridad_maxima, mem->prioridad_maxima_otro_nodo);
    #endif
    sem_post(&(mem->sem_prioridad_maxima));
    sem_post(&(mem->sem_prioridad_maxima_otro_nodo));
}
```

---

### File: procesos.h - Función send_testigo()

**Línea 280-320 - PROBLEMA: BREAKS SIN LIBERAR SEMÁFOROS**

**ANTES (INCORRECTO):**
```c
for(j= P -1; j>=0; j--){
    id_buscar = id_inicio;
    for(i=0; i < mem->num_nodos; i++){
        if(id_buscar != mi_id){
            sem_wait(&(mem->sem_peticiones));      // 1
            sem_wait(&(mem->sem_atendidas));       // 2
            
            if(mem->peticiones[id_buscar-1][j] > mem->atendidas[id_buscar-1][j]){
                sem_post(&(mem->sem_atendidas));   
                sem_post(&(mem->sem_peticiones));  
                encontrado = 1;
                break;  // ⚠️ PROBLEMA
            }
            sem_post(&(mem->sem_atendidas));
            sem_post(&(mem->sem_peticiones));
        }
        id_buscar++;
    }
    if(encontrado) break;  // ⚠️ PROBLEMA
}
```

**DESPUÉS (CORRECTO):**
```c
for(j= P -1; j>=0 && !encontrado; j--){
    id_buscar = id_inicio;
    for(i=0; i < mem->num_nodos && !encontrado; i++){
        if(id_buscar != mi_id){
            sem_wait(&(mem->sem_peticiones));      // 1
            sem_wait(&(mem->sem_atendidas));       // 2
            
            if(mem->peticiones[id_buscar-1][j] > mem->atendidas[id_buscar-1][j]){
                // Liberar ANTES de salir del loop
                sem_post(&(mem->sem_atendidas));   
                sem_post(&(mem->sem_peticiones));  
                encontrado = 1;
                // NO break, las condiciones del loop evitarán la siguiente iteración
            } else {
                sem_post(&(mem->sem_atendidas));
                sem_post(&(mem->sem_peticiones));
            }
        }
        id_buscar++;
    }
}
```

---

## 📝 CHECKLIST DE VALIDACIÓN

Después de aplicar los fixes, verifica:

- [ ] NO hay `sem_wait` sin su correspondiente `sem_post` en TODOS los caminos
- [ ] Los `sem_post` están en ORDEN INVERSO a los `sem_wait`
- [ ] Las variables compartidas se LEEN cuando el semáforo está tomado
- [ ] NO hay loops/breaks que eviten `sem_post`
- [ ] NO se retienen semáforos durante I/O (msgsnd, msgrcv, usleep)
- [ ] Cada ruta de código mantiene simetría de sem_wait/post

## 🧪 PRUEBA CON HELGRIND

Después de las correcciones, ejecuta:

```bash
valgrind --tool=helgrind --history-level=full \
  --suppressions=helgrind-suppressions.txt \
  ./inicio prueba.config
```

Esto detectará automáticamente:
- Deadlocks
- Race conditions
- Problemas de sincronización
- Uso incorrecto de semáforos

