./matarProcesos.sh
# Borrar todos los segmentos de memoria compartida (Shared Memory)
for id in $(ipcs -m | awk 'NR > 3 {print $2}'); do
    ipcrm -m $id
done

# Borrar todos los semáforos (Semaphores)
for id in $(ipcs -s | awk 'NR > 3 {print $2}'); do
    ipcrm -s $id
done

# Borrar todas las colas de mensajes (Message Queues)
for id in $(ipcs -q | awk 'NR > 3 {print $2}'); do
    ipcrm -q $id
done

./compila.sh

echo "Limpieza de IPC completada. Procesos eliminados. Recompilado Ok"
