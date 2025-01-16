#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

#define KasjerKanal 1
#define NUM_CLIENTS 10


struct message {
    long mtype; // Typ wiadomości
    pid_t pid;  // PID procesu (klienta lub kasjera)
};