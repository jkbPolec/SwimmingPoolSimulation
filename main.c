#include "common.h"

void createCashier();
void createClients();


int main() {
    int shKey, shmid;
    struct RecreationPoolStruct *rpoolStruct;

    //-------------------Tworzenie pamieci dzielonej basenu rekreacyjnego
    shKey = ftok("shmfile", 65);
    if (shKey == -1) {
        perror("ftok");
        exit(1);
    }

    shmid = shmget(shKey, sizeof(struct RecreationPoolStruct), 0600 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    rpoolStruct = (struct RecreationPoolStruct *) shmat(shmid, NULL, 0);
    if (rpoolStruct == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    rpoolStruct->client_count = 10;
    rpoolStruct->total_age = 40;
    rpoolStruct->pids[0] = 1234;

    printf("Zapisano w pamięci dzielonej:\n");
    printf("Client count: %d\n", rpoolStruct->client_count);
    printf("Total age: %d\n", rpoolStruct->total_age);
    printf("Message: %d\n", rpoolStruct->pids[0]);


    //-------------------Tworzenie kasjera
    createCashier();

    //-------------------Tworzenie klientow
    createClients();

    //-------------------Czekanie az wszystkie procesy sie skoncza
    for (int i = 0; i < NUM_CLIENTS + 1; i++) {
        wait(NULL);
    }

    printf("Wszystkie procesy zakończone.\n");

    return 0;
}

void createCashier() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed for kasjer");
        exit(1);
    }

    if (pid == 0) {
        printf("Tworzę proces kasjera...\n");
        execl("./kasjer", "kasjer", (char *)NULL);
        perror("execl kasjer failed");
        exit(1);
    }
}

void createClients() {
    pid_t pid;
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed for klient");
            exit(1);
        }

        if (pid == 0) {
            printf("Tworzę proces klienta %d...\n", i + 1);
            execl("./klient", "klient", (char *)NULL);
            perror("execl klient failed");
            exit(1);
        }
    }
}

