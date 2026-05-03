# ARQUITECTURA RECOMENDADA Y BUENAS PRÁCTICAS

## 🏗️ PROBLEMAS FUNDAMENTALES DE LA ARQUITECTURA ACTUAL

### Problema 1: Excesiva Complejidad en Sincronización
**Síntoma**: Múltiples semáforos para una sola operación lógica  
**Causa**: Intento de granularidad muy fina en el control

```c
// ❌ ACTUAL: 5 semáforos para una sola decisión
sem_wait(&(mem->sem_contador_pag_adm_pendientes));
sem_wait(&(mem->sem_testigo));
sem_wait(&(mem->sem_turno_PAG_ADM));
sem_wait(&(mem->sem_turno));
sem_wait(&(mem->sem_contador_procesos_max_SC));
// ... lógica ...
```

**Impacto**: Fácil crear deadlocks, difícil de razonar, mantenimiento de pesadilla

---

### Problema 2: Mezcla de IPC Mechanisms

```c
// Usa TRES mecanismos IPC simultáneamente:
// 1. Memoria compartida (shm)
// 2. Semáforos (sem)
// 3. Message queues (msg)
```

**Complejidad**: Cada mecanismo tiene sus propias garantías
- Semáforos: problemas de prioridad, inversión de prioridad
- Message queues: overhead de copia
- Shared memory: race conditions si fallan semáforos

---

### Problema 3: Testigos Circulantes Distribuidos

El protocolo de testigos es complejo:
- Testigos maestros + copias
- Cambio dinámico de nodo maestro
- Múltiples cambios de turno
- Prioridades dinámicas

**Complejidad del razonamiento**: Muy alto

---

## ✅ ARQUITECTURA RECOMENDADA - Opción 1: Token Ring Simplificado

### Conceptos Clave
- Un único token que circula
- Sin testigos copia
- Cambio de nodo maestro determinístico
- Prioridades estáticas

### Ventajas
- Más simple de razonar
- Menos semáforos
- Menos likelihood de deadlock

### Implementación

```c
// procesos_v2.h - Versión simplificada

#define NUM_NODOS 100

// Token circular simple
typedef struct {
    int nodo_actual;          // Quién tiene el token
    int secuencia;            // Número del token para validación
} token_simple;

// Buffer para mensaje
typedef struct {
    long msg_type;
    int id_remitente;
    int prioridad;
    int secuencia;
    char datos[256];
} mensaje_simple;

// Memoria compartida ultra-simple
typedef struct {
    int num_nodos;
    int tiempo_SC;
    
    // Solo lo que realmente necesitas
    int token_holder;         // Quién tiene el token
    int token_seq;            // Secuencia del token
    
    // Contadores de peticiones por prioridad
    int peticiones[NUM_NODOS][4];  // [nodo][prioridad]
    int atendidas[NUM_NODOS][4];   // [nodo][prioridad]
    
    // Un único semáforo general
    sem_t sem_general;
    
} memoria_simple;
```

### Flujo de Operación

```
1. Proceso quiere acceder a SC
   → Incrementa contador peticiones[mi_id][prioridad]
   → Espera en sem_general

2. Token holder recibe petición vía message queue
   → Lee petición
   → Determina siguiente nodo (máxima prioridad pendiente)
   → Envía token a ese nodo vía message queue

3. Nuevo token holder
   → Entra a SC si es su turno
   → Sale y decide siguiente destino
   → Envía token
```

**Código simplificado de entrada a SC:**

```c
int main(int argc, char* argv[]) {
    int mi_id = atoi(argv[1]);
    memoria_simple *mem;
    int memoria_id = shmget(mi_id, sizeof(memoria_simple), 0);
    mem = shmat(memoria_id, NULL, 0);
    
    // Registrar petición
    sem_wait(&mem->sem_general);
    mem->peticiones[mi_id-1][PRIORIDAD]++;
    sem_post(&mem->sem_general);
    
    // Enviar petición a token holder
    mensaje_simple msg = {
        .msg_type = 1,
        .id_remitente = mi_id,
        .prioridad = PRIORIDAD,
        .secuencia = mem->peticiones[mi_id-1][PRIORIDAD]
    };
    
    int token_holder_queue = mem->buzones_nodos[mem->token_holder - 1];
    msgsnd(token_holder_queue, &msg, sizeof(msg) - sizeof(long), 0);
    
    // Esperar token
    int mi_buzon = mem->buzones_nodos[mi_id - 1];
    msgrcv(mi_buzon, &msg, sizeof(msg) - sizeof(long), 0, 0);
    
    // Tenemos el token - SECCIÓN CRÍTICA
    printf("Proceso %d en sección crítica\n", mi_id);
    usleep(mem->tiempo_SC);
    
    // Salir y pasar token
    // ... lógica de selección de siguiente nodo ...
    
    return 0;
}
```

---

## ✅ ARQUITECTURA RECOMENDADA - Opción 2: Distributed Mutex con Timestamps

### Concepto
Algoritmo de Lamport o Vector Clocks para exclusión mutua distribuida

### Ventajas
- Sin necesidad de testigos
- Matemáticamente probado
- Escalable

### Implementación Simple

```c
// Estructura de mensajes
typedef struct {
    long msg_type;           // 1=REQUEST, 2=REPLY
    int id_nodo;
    int timestamp;           // Timestamp lógico
    int prioridad;
    int secuencia;
} mensaje_lamport;

// Estructura compartida
typedef struct {
    int num_nodos;
    int tiempo_SC;
    int logical_clock;       // Reloj lógico del nodo
    
    int peticiones_pendientes[NUM_NODOS][4];  // Peticiones por nodo/prioridad
    int replies_recibidos[NUM_NODOS];         // Replies recibidos
    
    sem_t sem_general;
} memoria_lamport;

// Algoritmo: Petición de entrada
int solicitar_entrada(int mi_id, memoria_lamport *mem, int prioridad) {
    
    // 1. Increment logical clock
    sem_wait(&mem->sem_general);
    mem->logical_clock++;
    int mi_timestamp = mem->logical_clock;
    mem->peticiones_pendientes[mi_id-1][prioridad]++;
    sem_post(&mem->sem_general);
    
    // 2. Enviar REQUEST a todos
    for (int i = 0; i < mem->num_nodos; i++) {
        if (i == mi_id - 1) continue;
        
        mensaje_lamport msg = {
            .msg_type = 1,  // REQUEST
            .id_nodo = mi_id,
            .timestamp = mi_timestamp,
            .prioridad = prioridad
        };
        
        int queue_id = mem->buzones_nodos[i];
        msgsnd(queue_id, &msg, sizeof(msg) - sizeof(long), 0);
    }
    
    // 3. Esperar replies de todos
    int replies_recibidos = 0;
    while (replies_recibidos < mem->num_nodos - 1) {
        // Implementar timeout
        // Recibir y procesar replies
    }
    
    // 4. Entrar a SC
    return 0;
}
```

---

## 🛡️ BUENAS PRÁCTICAS DE CONCURRENCIA

### 1. Principio: "Lock Duration" (Mantén locks el menor tiempo posible)

**❌ MALO:**
```c
sem_wait(&mutex);
// Adquires lock
int valor = compartida->valor;
msgsnd(...);  // ⚠️ I/O LENTO aquí dentro del lock
sem_post(&mutex);
```

**✅ BUENO:**
```c
sem_wait(&mutex);
int valor = compartida->valor;  // Lectura rápida
sem_post(&mutex);  // Libera inmediatamente
msgsnd(...);  // I/O FUERA del lock
```

---

### 2. Principio: "Copy-On-Read"

**❌ MALO:**
```c
sem_wait(&mutex);
if (mem->contador > 10 && mem->estado == ACTIVO) {
    // ... hacer algo ...
}
sem_post(&mutex);
```

**✅ BUENO:**
```c
int contador_local, estado_local;
sem_wait(&mutex);
contador_local = mem->contador;
estado_local = mem->estado;
sem_post(&mutex);

if (contador_local > 10 && estado_local == ACTIVO) {
    // ... hacer algo sin lock ...
}
```

---

### 3. Principio: "Hierarchical Locking"

Define orden global de adquisición:

```c
// ORDEN GLOBAL FIJO
#define LOCK_ORDER \
    X(SEM_DATOS),     /* 1 */ \
    X(SEM_TURNO),     /* 2 */ \
    X(SEM_ESTADO),    /* 3 */ \
    X(SEM_CONTADOR)   /* 4 */
```

**SIEMPRE** adquirir en este orden, **NUNCA** salirse.

---

### 4. Principio: "Semaphore Wrappers"

**❌ MALO:**
```c
// Semáforos desnudos esparcidos por el código
sem_wait(&mem->sem_algo);
mem->variable++;
sem_post(&mem->sem_algo);
```

**✅ BUENO:**
```c
void increment_variable(memoria_nodo *mem) {
    sem_wait(&mem->sem_algo);
    mem->variable++;
    sem_post(&mem->sem_algo);
}

// Uso
increment_variable(mem);
```

---

### 5. Principio: "Guard Clauses"

**❌ MALO:**
```c
sem_wait(&mutex);
if (condicion1) {
    if (condicion2) {
        if (condicion3) {
            // Trabajo
            sem_post(&mutex);
            return;
        }
    }
}
sem_post(&mutex);  // ⚠️ Puede no ejecutarse
```

**✅ BUENO:**
```c
sem_wait(&mutex);

// Guard clauses
if (!condicion1) {
    sem_post(&mutex);
    return;
}
if (!condicion2) {
    sem_post(&mutex);
    return;
}
if (!condicion3) {
    sem_post(&mutex);
    return;
}

// Trabajo
sem_post(&mutex);
```

---

## 📚 Patrones Recomendados

### Patrón 1: RAII (Resource Acquisition Is Initialization)

```c
// Macro para C
#define SEM_ACQUIRE(sem, cleanup) \
    sem_t *_sem_acquired = NULL; \
    if (sem_wait(sem) == 0) _sem_acquired = sem; \
    else { cleanup; goto cleanup_label; }

#define SEM_RELEASE() \
    if (_sem_acquired) sem_post(_sem_acquired); \
    cleanup_label:

// Uso
SEM_ACQUIRE(&mutex, { return -1; })
// ... código ...
SEM_RELEASE()
```

---

### Patrón 2: Try-Lock with Timeout

```c
int sem_wait_timeout(sem_t *sem, int seconds) {
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += seconds;
    
    return sem_timedwait(sem, &timeout);
}

// Uso
if (sem_wait_timeout(&mutex, 5) == -1) {
    printf("Timeout or error - possible deadlock!\n");
    return -1;  // Detectar deadlock
}
```

---

### Patrón 3: State Machine para Flujos Complejos

```c
typedef enum {
    STATE_IDLE,
    STATE_REQUESTING,
    STATE_WAITING,
    STATE_EXECUTING,
    STATE_DONE
} proceso_state;

typedef struct {
    proceso_state state;
    int transition_time;
    // ...
} proceso_context;

// Máquina de estados
while (ctx->state != STATE_DONE) {
    switch(ctx->state) {
        case STATE_IDLE:
            ctx->state = STATE_REQUESTING;
            break;
        case STATE_REQUESTING:
            // Lógica de petición
            ctx->state = STATE_WAITING;
            break;
        case STATE_WAITING:
            // Esperar respuesta
            if (respuesta_recibida) {
                ctx->state = STATE_EXECUTING;
            }
            break;
        case STATE_EXECUTING:
            // Sección crítica
            ctx->state = STATE_DONE;
            break;
    }
    usleep(100000);  // Yield
}
```

**Ventajas**: 
- Fácil de razonar
- Fácil de depurar
- Visible el flujo
- Menos anidamiento condicional

---

## 🎯 RECOMENDACIÓN FINAL

Para este proyecto:

1. **Corto plazo (Urgente - 1-2 días)**: 
   - Aplicar los fixes específicos del documento SOLUCIONES_ESPECIFICAS.md
   - Añadir testing con Helgrind
   - Documentar el protocolo actual

2. **Medio plazo (1-2 semanas)**:
   - Refactorizar para usar máquina de estados
   - Reducir número de semáforos
   - Implementar try-lock con timeout

3. **Largo plazo (1-2 meses)**:
   - Considerar Opción 1: Token Ring Simplificado
   - O Opción 2: Distributed Mutex con Timestamps
   - Refactorización completa con nueva arquitectura

---

## 📖 Referencias Recomendadas

1. **"The Little Book of Semaphores"** (Allen B. Downey)
   - Excelente introducción a problemas clásicos
   - Soluciones probadas

2. **"Concurrent Programming in Java"** (Doug Lea)
   - Principios aplicables a C también
   - Patrones robustos

3. **"Distributed Algorithms"** (Nancy A. Lynch)
   - Algoritmos distribuidos probados
   - Token rings, Lamport clocks

4. **POSIX Threads Standard**
   - Documentación oficial
   - Garantías de sincronización

