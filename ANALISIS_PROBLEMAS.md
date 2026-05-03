# Análisis de Problemas - Sistema Concurrente Distribuido

## 🔴 PROBLEMAS CRÍTICOS ENCONTRADOS

### 1. **DEADLOCK POR ORDEN INCONSISTENTE DE SEMÁFOROS**
**Severidad**: CRÍTICA  
**Archivos**: `pagos.c`, `administraciones.c`, `anulaciones.c`, `consultas.c`

**Problema**:
```c
// En pagos.c líneas ~35-40 (Orden A):
sem_wait(&(mem->sem_contador_pag_adm_pendientes));  // 1
sem_wait(&(mem->sem_testigo));                       // 2
sem_wait(&(mem->sem_turno_PAG_ADM));                 // 3
sem_wait(&(mem->sem_turno));                         // 4
sem_wait(&(mem->sem_contador_procesos_max_SC));      // 5

// Pero luego el código hace diferentes post en diferentes ramas
// Procesps posteriores puede adquirir semáforos en orden diferente
```

**Por qué es problema**:
- Si Proceso A adquiere sem_1 → sem_2 y espera sem_3
- Y Proceso B adquiere sem_3 → sem_2 y espera sem_1
- Se produce DEADLOCK circular

**Solución**: Usar **SIEMPRE el mismo orden** para adquirir semáforos:
1. Definir un orden global: contador → testigo → turno → procesos_max_SC
2. TODOS los sem_wait deben seguir este orden
3. Los sem_post deben ser en orden INVERSO

---

### 2. **SEM_POST INCOMPLETO - Semáforos sin liberar**
**Severidad**: CRÍTICA  
**Archivos**: Todos los procesos (.c)

**Problema en pagos.c líneas ~35-85**:
```c
sem_wait(&(mem->sem_contador_pag_adm_pendientes));  // 1
sem_wait(&(mem->sem_testigo));                       // 2
sem_wait(&(mem->sem_turno_PAG_ADM));                 // 3
sem_wait(&(mem->sem_turno));                         // 4
sem_wait(&(mem->sem_contador_procesos_max_SC));      // 5

if (condition) {
    // Rama 1: libera todos los 5
    sem_post(&(mem->sem_testigo));
    sem_post(&(mem->sem_turno_PAG_ADM));
    sem_post(&(mem->sem_contador_procesos_max_SC));
    sem_post(&(mem->sem_turno));
    sem_post(&(mem->sem_contador_pag_adm_pendientes));
    
    // ... resto de código
    sem_wait(&(mem->sem_pag_adm_pend));  // ⚠️ ESPERA AQUÍ

} else {
    // Rama 2: libera todos
    sem_post(&(mem->sem_testigo));
    sem_post(&(mem->sem_turno_PAG_ADM));
    
    // ... más código
    // ❌ PROBLEMA: Si falla, algunos sem_wait quedan sin liberar
}
```

**Consecuencia**: Semáforos quedan bloqueados indefinidamente, causando que otros procesos se queden esperando.

---

### 3. **LECTURA DE VARIABLES SIN SINCRONIZACIÓN**
**Severidad**: CRÍTICA  
**Archivos**: `pagos.c`, `receptor.c`, `administraciones.c`

**Problema en pagos.c línea ~38-44**:
```c
sem_wait(&(mem->sem_contador_pag_adm_pendientes));  // Adquiere semáforo
mem->contador_pag_adm_pendientes++;
// ... 4 sem_wait más

if ((!mem->testigo && mem->contador_pag_adm_pendientes == 1) ||  
    // ❌ Se leen `testigo` y `contador_pag_adm_pendientes` SIN semáforo
    (mem->testigo && mem->turno_PAG_ADM && ...)) {
    // ...
}
```

**Por qué es problema**:
- El semáforo se libera pero luego se leen las variables sin protección
- Otro proceso puede cambiar estas variables entre la lectura y la evaluación
- Esto causa **race conditions**

---

### 4. **RACE CONDITION EN receptor.c**
**Severidad**: ALTA  
**Archivo**: `receptor.c` líneas ~48-70

**Problema**:
```c
if(mensaje_rx.prioridad == CONSULTA){
    sem_wait(&mem->sem_prioridad_maxima);
    sem_wait(&mem->sem_prioridad_maxima_otro_nodo);
    sem_wait(&mem->sem_testigo);

    if(mem->testigo == 1 && 
       mem->prioridad_maxima_otro_nodo == CONSULTA &&  // ❌ Se lee sin tener el semáforo
       (mem->prioridad_maxima == CONSULTA || mem->prioridad_maxima == 0)) {
        // ...
    }
    // ...
}
```

**Problema**: Se hace sem_wait DESPUÉS de leer una variable, cuando debería ser ANTES.

---

### 5. **FUNCIÓN send_testigo() CON SEMÁFOROS SIN LIBERAR**
**Severidad**: ALTA  
**Archivo**: `procesos.h` líneas ~246-340

**Problema**:
```c
void send_testigo(int mi_id, memoria_nodo *mem){
    // ... código ...
    
    for(j= P -1; j>=0; j--){
        id_buscar = id_inicio;
        for(i=0; i < mem->num_nodos; i++){
            if(id_buscar != mi_id){
                sem_wait(&(mem->sem_peticiones));      // 1
                sem_wait(&(mem->sem_atendidas));       // 2
                
                if(mem->peticiones[id_buscar-1][j] > mem->atendidas[id_buscar-1][j]){
                    sem_post(&(mem->sem_atendidas));   // Libera 2
                    sem_post(&(mem->sem_peticiones));  // Libera 1
                    encontrado = 1;
                    // ... más código ...
                    break;  // ⚠️ Sale del loop interno
                }
                sem_post(&(mem->sem_atendidas));
                sem_post(&(mem->sem_peticiones));
            }
            id_buscar++;
        }
        if(encontrado){
            break;  // ⚠️ Sale del loop externo sin liberar semáforos en algunos casos
        }
    }
```

**Problema**: Los breaks pueden causar que salga de los bucles sin liberar ciertos semáforos en algunos caminos de ejecución.

---

### 6. **VENTANA DE RACE CONDITION EN calcular_prioridad_maxima()**
**Severidad**: ALTA  
**Archivo**: `procesos.h` líneas ~184-240

**Problema**:
```c
void calcular_prioridad_maxima(memoria_nodo *mem){
    sem_wait(&(mem->sem_contador_anul_pendientes));
    if(mem->contador_anul_pendientes > 0){
        sem_post(&(mem->sem_contador_anul_pendientes));  // Libera
        // ⚠️ VENTANA: Otro proceso puede cambiar contador_anul_pendientes aquí
        sem_wait(&(mem->sem_prioridad_maxima));
        mem->prioridad_maxima = ANUL;
        sem_post(&(mem->sem_prioridad_maxima));
    } else {
        sem_post(&(mem->sem_contador_anul_pendientes));  // Libera
        // ⚠️ VENTANA: Otro proceso puede cambiar el contador aquí también
        sem_wait(&(mem->sem_contador_pag_adm_pendientes));
        // ...
    }
}
```

---

### 7. **POSIBLE DEADLOCK EN MÚLTIPLES SEM_WAIT**
**Severidad**: ALTA  
**Archivos**: `pagos.c` líneas ~35-40, similar en otros procesos

**Problema del patrón**:
```c
sem_wait(&A);
sem_wait(&B);
sem_wait(&C);
sem_wait(&D);
sem_wait(&E);

if (condition_A) {
    sem_post(&A); sem_post(&B); sem_post(&C); sem_post(&D); sem_post(&E);
    // Código que puede hacer sem_wait(&A) nuevamente
    sem_wait(&F);  // ⚠️ Espera aquí
}
```

Si en paralelo otro proceso está esperando A y hace sem_wait en orden diferente, deadlock.

---

## 📊 TABLA RESUMEN DE PROBLEMAS

| ID | Problema | Severidad | Archivo | Líneas | Impacto |
|---|---|---|---|---|---|
| 1 | Orden inconsistente semáforos | CRÍTICA | Todos .c | 35-40 | Deadlock |
| 2 | Sem_post incompleto | CRÍTICA | Todos .c | Variable | Semáforos atrapados |
| 3 | Lectura sin sincronización | CRÍTICA | pagos.c | 38-50 | Race condition |
| 4 | Lectura después sem_wait | ALTA | receptor.c | 48-70 | Race condition |
| 5 | send_testigo() sin liberar | ALTA | procesos.h | 280-320 | Bloqueos |
| 6 | Ventana en calcular_prioridad | ALTA | procesos.h | 184-240 | Race condition |
| 7 | Deadlock por orden variable | ALTA | Todos .c | Variable | Deadlock |

---

## 🔧 RECOMENDACIONES DE SOLUCIÓN

### Corto plazo (Fixes inmediatos):

1. **Implementar mutex de orden global**:
   ```c
   // Define un orden fijo para TODOS los semáforos:
   // 1. sem_contador_* (contadores)
   // 2. sem_testigo
   // 3. sem_turno_*
   // 4. sem_procesos_max_SC
   // 5. sem_dentro
   // 6. sem_atendidas
   // 7. sem_peticiones
   ```

2. **Usar try-catch con timeout o restructurar**:
   ```c
   // Envolver en función helper que garantice sem_post
   void safe_critical_section(memoria_nodo *mem, 
                              int (*func)(memoria_nodo*),
                              sem_t **locks, int count) {
       // Adquiere todos en orden
       // Ejecuta función
       // Libera todos en orden inverso GARANTIZADO
   }
   ```

3. **Añadir validación en cada rama**:
   ```c
   // Usar bandera para verificar qué semáforos están activos
   int sem_mask = 0;
   if(need_sem_contador) {
       sem_wait(&(mem->sem_contador));
       sem_mask |= 0x01;
   }
   // ... guardar estado ...
   
   // Al final, liberar según máscara
   if(sem_mask & 0x01) sem_post(&(mem->sem_contador));
   ```

### Largo plazo (Refactoring):

1. **Considerar usar un Reader-Writer lock** en lugar de múltiples semáforos
2. **Usar herramientas como ThreadSanitizer (TSAN)** para detectar race conditions automáticamente
3. **Implementar modelo de actores** o **message-passing puro** sin memoria compartida
4. **Usar mutexes con guardia automática** (RAII patterns)

---

## 🧪 CÓMO REPRODUCIR LOS PROBLEMAS

1. **Deadlock**: Ejecutar con múltiples nodos y múltiples procesos simultáneamente
   ```bash
   ./inicio prueba.config
   # El sistema probablemente se colgará después de algunos segundos
   ```

2. **Race conditions**: Ejecutar bajo stress con:
   ```bash
   valgrind --tool=helgrind --history-level=full ./programa
   # O
   clang -fsanitize=thread ./programa.c -o programa
   ```

