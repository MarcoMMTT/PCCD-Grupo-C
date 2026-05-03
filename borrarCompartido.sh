#!/bin/bash

if [ $# -ne 2]; then
    echo "Argumentos: id de memoria compartida 1, id memoria compartida último"
    exit 1
fi

if [ $1 -ge $2 ]; then
    echo "El primer argumento debe ser mas pequeño que el segundo"
fi

while [ $i -le $2 ]
do
    sudo ipcrm -m $i
    sudo ipcrm -q $i
    i=$((i+1))
done

echo "MEMORIA COMPARTIDA ELIMINADA"

exit 0