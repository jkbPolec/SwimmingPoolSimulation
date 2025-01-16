#!/bin/bash

echo "Usuwanie wszystkich kolejek komunikatów, semaforów i segmentów pamięci dzielonej..."

# Usuwanie wszystkich kolejek komunikatów
for queue_id in $(ipcs -q | awk 'NR>3 {print $2}'); do
    ipcrm -q "$queue_id"
    echo "Usunięto kolejkę komunikatów o ID: $queue_id"
done

# Usuwanie wszystkich semaforów
for sem_id in $(ipcs -s | awk 'NR>3 {print $2}'); do
    ipcrm -s "$sem_id"
    echo "Usunięto semafor o ID: $sem_id"
done

# Usuwanie wszystkich segmentów pamięci dzielonej
for shm_id in $(ipcs -m | awk 'NR>3 {print $2}'); do
    ipcrm -m "$shm_id"
    echo "Usunięto segment pamięci dzielonej o ID: $shm_id"
done

echo "Wszystkie zasoby IPC zostały usunięte."
