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
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>

#define SEM_KEY 47382

#define MAX_CLIENT_MSG 500

#define CASHIER_CHANNEL                 101

#define RECREATIONAL_ENTER_CHANNEL  211
#define RECREATIONAL_EXIT_CHANNEL  212
#define KIDS_ENTER_CHANNEL          221
#define KIDS_EXIT_CHANNEL          222
#define OLYMPIC_ENTER_CHANNEL       231
#define OLYMPIC_EXIT_CHANNEL       232

#define RECREATIONAL_POOL_CODE          301
#define KIDS_POOL_CODE                  302
#define OLYMPIC_POOL_CODE               303

#define MANAGER_CHANNEL             401

#define EXIT_POOL_SIGNAL 35
#define CASHIER_SIGNAL 36
#define CLOSE_POOL_SIGNAL 34
#define OPEN_POOL_SIGNAL 35
#define PRINT_POOL_SIGNAL 36

#define NUM_CLIENTS 1000
#define RECREATIONAL_POOL_SIZE          50
#define KIDS_POOL_SIZE                  30
#define OLYMPIC_POOL_SIZE              30
#define MAX_AGE 40

struct message {
    long mtype;         // Typ wiadomości (do kasjera lub klienta)
    pid_t pid;          // PID procesu (klienta lub kasjera)
    int allowed;        //1 - akcja dozwolona, 0 - zabroniona
    int kidAge;
};

struct LifeguardMessage {
    long mtype;         // Typ wiadomości (do ratownika lub klienta)
    pid_t pid;
    int age;
    int allowed;        //1 - akcja dozwolona, 0 - zabroniona
    bool hasKid;
    int kidAge;
};

struct PoolStruct {
    int client_count;
    int total_age;
    int pids[];
    };

struct PoolStatus {
    bool isOpened;      //1 - basen otwarty, 0 - basen zakmniety
};





