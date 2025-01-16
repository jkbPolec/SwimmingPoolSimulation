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
#include <stdbool.h>

#define CASHIER_CHANNEL 1
#define NUM_CLIENTS 10
#define VIP_CODE 1
#define R_POOL_SIZE 100

struct message {
    long mtype; // Typ wiadomości
    pid_t pid;  // PID procesu (klienta lub kasjera)
};

struct RecreationPoolStruct {
    int client_count;
    int total_age;
    int pids[R_POOL_SIZE];
};

