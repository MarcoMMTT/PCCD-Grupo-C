# HERRAMIENTAS DE VALIDACIÓN Y TESTING

## 🧪 Script para Detectar Problemas Automáticamente

### 1. Compilar con ThreadSanitizer (Detecta race conditions)

```bash
# Crea nuevo compilación con TSAN
clang -fsanitize=thread -g -O1 \
  -I. pagos.c inicio.c receptor.c -o pagos_debug

# O con gcc
gcc -fsanitize=thread -g -O1 \
  -I. pagos.c inicio.c receptor.c -o pagos_debug

# Ejecutar
./pagos_debug
```

**Qué detecta**: Acceso a variables compartidas desde múltiples threads sin sincronización.

---

### 2. Valgrind + Helgrind (Detecta problemas de threading)

```bash
# Instalar si no está
sudo apt install valgrind

# Ejecutar con Helgrind
valgrind --tool=helgrind --read-var-info=yes \
  --history-level=full \
  ./inicio prueba.config

# Salida típica de deadlock:
# "Lock order violated"
# "Potential deadlock"
```

---

### 3. Script para Verificar Orden de Semáforos

Crear archivo: [check_semaphores.sh](check_semaphores.sh)

```bash
#!/bin/bash

# Analizar código C para encontrar patrones peligrosos de semáforos

echo "=== BÚSQUEDA DE PATRONES PELIGROSOS ==="

# 1. Encontrar múltiples sem_wait sin orden consistente
echo "1. Detectando múltiples sem_wait consecutivos..."
grep -n "sem_wait.*sem_wait" *.c 2>/dev/null || echo "   ✓ No encontrados"

# 2. Verificar sem_post sin protección anterior
echo "2. Buscando sem_post sin sem_wait previo..."
grep -B5 "sem_post" *.c | grep -v "sem_wait" | tail -n 5

# 3. Detectar loops con sem_wait/sem_post
echo "3. Detectando potenciales deadlocks en loops..."
grep -n "for.*{" *.c | while read line; do
    num=$(echo "$line" | cut -d: -f1)
    echo "   Revisar línea: $num"
done

# 4. Contar sem_wait vs sem_post
echo "4. Conteo de sem_wait vs sem_post por archivo:"
for f in *.c; do
    wait_count=$(grep -c "sem_wait" "$f" 2>/dev/null || echo 0)
    post_count=$(grep -c "sem_post" "$f" 2>/dev/null || echo 0)
    echo "   $f: wait=$wait_count, post=$post_count"
    if [ $wait_count -ne $post_count ]; then
        echo "      ⚠️  DESBALANCE DETECTADO!"
    fi
done
```

---

### 4. Programa de Prueba: Detector de Deadlock

Crear archivo: [deadlock_detector.c](deadlock_detector.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define TIMEOUT_SECONDS 5

typedef struct {
    sem_t sem1, sem2;
    int thread_id;
} test_data;

void* thread_normal(void* arg) {
    test_data* data = (test_data*)arg;
    printf("Thread %d: Adquiriendo sem1...\n", data->thread_id);
    sem_wait(&data->sem1);
    usleep(100000);  // Simula trabajo
    printf("Thread %d: Adquiriendo sem2...\n", data->thread_id);
    sem_wait(&data->sem2);
    printf("Thread %d: ¡Éxito! Ambos semáforos adquiridos\n", data->thread_id);
    sem_post(&data->sem2);
    sem_post(&data->sem1);
    return NULL;
}

void* thread_reverse(void* arg) {
    test_data* data = (test_data*)arg;
    printf("Thread %d: Adquiriendo sem2...\n", data->thread_id);
    sem_wait(&data->sem2);
    usleep(100000);  // Simula trabajo
    printf("Thread %d: Adquiriendo sem1...\n", data->thread_id);
    sem_wait(&data->sem1);
    printf("Thread %d: ¡Éxito! Ambos semáforos adquiridos\n", data->thread_id);
    sem_post(&data->sem1);
    sem_post(&data->sem2);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <test_type>\n", argv[0]);
        printf("  1 = Test correcto (sin deadlock)\n");
        printf("  2 = Test incorrecto (deadlock)\n");
        return 1;
    }

    int test_type = atoi(argv[1]);
    test_data data;
    pthread_t t1, t2;
    time_t start_time = time(NULL);

    sem_init(&data.sem1, 0, 1);
    sem_init(&data.sem2, 0, 1);

    printf("=== Iniciando test tipo %d ===\n", test_type);

    if (test_type == 1) {
        // Orden consistente: ambos threads en mismo orden
        printf("Test CORRECTO: Orden consistente\n");
        data.thread_id = 1;
        pthread_create(&t1, NULL, thread_normal, &data);
        usleep(500000);
        data.thread_id = 2;
        pthread_create(&t2, NULL, thread_normal, &data);
    } else {
        // Orden inconsistente: threads en orden opuesto
        printf("Test INCORRECTO: Orden inconsistente (¡PELIGRO DEADLOCK!)\n");
        data.thread_id = 1;
        pthread_create(&t1, NULL, thread_normal, &data);
        data.thread_id = 2;
        pthread_create(&t2, NULL, thread_reverse, &data);
    }

    // Esperar con timeout
    for (int i = 0; i < TIMEOUT_SECONDS; i++) {
        sleep(1);
        time_t elapsed = time(NULL) - start_time;
        printf("[%ld segundos]\n", elapsed);
        if (elapsed >= TIMEOUT_SECONDS - 1) {
            printf("\n⚠️  TIMEOUT: Posible DEADLOCK detectado\n");
            break;
        }
    }

    // Intentar cleanup (puede fallar si hay deadlock)
    printf("\nIntentando cleanup...\n");
    pthread_cancel(t1);
    pthread_cancel(t2);

    sem_destroy(&data.sem1);
    sem_destroy(&data.sem2);

    return 0;
}
```

**Compilar y ejecutar:**
```bash
gcc -pthread -o deadlock_detector deadlock_detector.c
./deadlock_detector 1  # Test correcto - debe completar rápido
./deadlock_detector 2  # Test incorrecto - producirá timeout/deadlock
```

---

### 5. Script para Analizar Estructura de Semáforos

Crear: [analyze_semaphores.py](analyze_semaphores.py)

```python
#!/usr/bin/env python3

import re
import sys
from collections import defaultdict

def analyze_file(filename):
    """Analiza un archivo C en busca de patrones de semáforos"""
    
    with open(filename, 'r') as f:
        content = f.read()
        lines = content.split('\n')
    
    # Encontrar funciones
    functions = re.finditer(r'\b(\w+)\s*\([^)]*\)\s*\{', content)
    
    results = {
        'deadlock_risks': [],
        'sem_balances': defaultdict(lambda: {'wait': 0, 'post': 0}),
        'inconsistent_orders': [],
        'race_conditions': []
    }
    
    # Contar sem_wait vs sem_post
    for match in re.finditer(r'sem_wait\(&\(mem->(\w+)\)\)', content):
        sem_name = match.group(1)
        results['sem_balances'][sem_name]['wait'] += 1
    
    for match in re.finditer(r'sem_post\(&\(mem->(\w+)\)\)', content):
        sem_name = match.group(1)
        results['sem_balances'][sem_name]['post'] += 1
    
    # Detectar desbalances
    for sem, counts in results['sem_balances'].items():
        if counts['wait'] != counts['post']:
            results['deadlock_risks'].append(
                f"DESBALANCE: {sem} tiene {counts['wait']} wait y {counts['post']} post"
            )
    
    # Detectar múltiples sem_wait sin orden
    multi_wait_pattern = r'sem_wait\([^)]+\)\s*;\s*sem_wait\([^)]+\)\s*;'
    for match in re.finditer(multi_wait_pattern, content):
        start_line = content[:match.start()].count('\n') + 1
        results['deadlock_risks'].append(
            f"MÚLTIPLES SEM_WAIT en línea {start_line}: Verificar orden"
        )
    
    return results

def main():
    if len(sys.argv) < 2:
        print("Uso: python3 analyze_semaphores.py <archivo.c>")
        sys.exit(1)
    
    for filename in sys.argv[1:]:
        print(f"\n=== Analizando {filename} ===")
        results = analyze_file(filename)
        
        print("\nBalances de semáforos:")
        for sem, counts in sorted(results['sem_balances'].items()):
            status = "✓" if counts['wait'] == counts['post'] else "✗"
            print(f"  {status} {sem}: wait={counts['wait']}, post={counts['post']}")
        
        if results['deadlock_risks']:
            print("\n⚠️  RIESGOS DETECTADOS:")
            for risk in results['deadlock_risks']:
                print(f"  - {risk}")
        else:
            print("\n✓ No se detectaron riesgos obvios")

if __name__ == '__main__':
    main()
```

**Ejecutar:**
```bash
python3 analyze_semaphores.py pagos.c administraciones.c receptor.c
```

---

## 📊 Tabla de Herramientas Recomendadas

| Herramienta | Detecta | Comando |
|---|---|---|
| **ThreadSanitizer** | Race conditions | `clang -fsanitize=thread` |
| **Helgrind** | Deadlocks, sincronización | `valgrind --tool=helgrind` |
| **AddressSanitizer** | Memory errors | `clang -fsanitize=address` |
| **Memorizer** | Memory leaks | `valgrind --tool=memcheck` |
| **DRD** | Data race detection | `valgrind --tool=drd` |

---

## 🚀 Workflow Recomendado de Testing

1. **Compilación segura:**
   ```bash
   gcc -Wall -Wextra -Wformat -Wshadow -g -O0 \
       -fsanitize=thread -pthread \
       pagos.c administraciones.c anulaciones.c \
       consultas.c receptor.c inicio.c -o app_debug
   ```

2. **Ejecución con monitoreo:**
   ```bash
   timeout 30 valgrind --tool=helgrind ./app_debug 2>&1 | tee test_output.log
   ```

3. **Análisis de salida:**
   ```bash
   grep -E "(ERROR|DEADLOCK|RACE|lock)" test_output.log
   ```

4. **Ejecución de stress test:**
   ```bash
   for i in {1..10}; do
       echo "Run $i..."
       timeout 10 ./app_debug prueba.config
   done
   ```

---

## 📋 Puntos de Verificación Manual

Además de herramientas automáticas, revisar manualmente:

1. **Cada `sem_wait` tiene correspondiente `sem_post`?**
   ```bash
   grep -n "sem_wait" pagos.c | wc -l
   grep -n "sem_post" pagos.c | wc -l
   # Deben ser iguales
   ```

2. **Las variables se leen con semáforos tomados?**
   ```bash
   grep -B2 "mem->" pagos.c | grep -c "sem_wait"
   # Alto número = más seguro
   ```

3. **Los semáforos se liberan en orden inverso?**
   - Búsqueda manual: encontrar bloques sem_wait/post
   - Verificar que post está en orden inverso

4. **No hay `break` o `return` sin liberar semáforos?**
   ```bash
   grep -n "break\|return" pagos.c
   # Revisar cada uno individualmente
   ```

