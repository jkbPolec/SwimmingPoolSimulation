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


#define CASHIER_CHANNEL                 101
#define RECREATIONAL_LIFEGAURD_CHANNEL  201
#define KIDS_LIFEGAURD_CHANNEL          202
#define OLYMPIC_LIFEGAURD_CHANNEL       203

#define RECREATIONAL_POOL_CODE          301
#define KIDS_POOL_CODE                  302
#define OLYMPIC_POOL_CODE               303

#define NUM_CLIENTS 3
#define VIP_CODE    1
#define RECREATIONAL_POOL_SIZE          100
#define KIDS_POOL_SIZE                  30
#define OLYMPIC_POOL_SIZE              50
#define MAX_AGE 40

struct message {
    long mtype; // Typ wiadomości (do kasjera lub klienta)
    pid_t pid;  // PID procesu (klienta lub kasjera)
};

struct LifeguardMessage {
    long mtype;          // Typ wiadomości (do ratownika lub klienta)
    pid_t pid;
    int age;
    int allowed;         //1 - wejście dozwolone, 0 - zabronione
};

struct PoolStruct {
    int client_count;
    int total_age;
    int pids[];
};

