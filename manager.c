#include "common.h"


pid_t cashier, lifeguardRec, lifeguardKid, lifeguardOly;
int shmid;
struct OpenHoursStruct *hoursStruct;
pid_t clientPIDs[1000];
int numberOfClients;
struct ManagerMessage mgrMsg;
pthread_t monitoringClients;


int msgid;
int key;

void SetupSharedMemory();
void SetClosingTime();
void AddClient(pid_t pid);
void RemoveClient(pid_t pid);
void *MonitorClients();

int main(int argc, char* argv[]) {


    if (argc != 5){
        fprintf(stderr,"Niepoprawna liczba argumentow!\n");
        exit(1);
    }

    cashier = (pid_t)atoi(argv[1]);
    lifeguardRec = (pid_t)atoi(argv[2]);
    lifeguardKid = (pid_t)atoi(argv[3]);
    lifeguardOly = (pid_t)atoi(argv[4]);

    int randomPid;
//    while (true) {
        //usleep(1250000);
//        randomPid = (pid_t)atoi(argv[rand() % 3 + 2]);
        //randomPid = 2;
//        printf("\033[1;31;40mWyslanie sygnalu zamkniecia\033[0m\n");

//        kill(cashier,CASHIER_SIGNAL);
//        usleep(5250000);
//        printf("\033[1;31;40mWyslanie sygnalu otwarcia\033[0m\n");
//        kill(cashier,CASHIER_SIGNAL);
//    }


    // Generowanie klucza do kolejki
     key = ftok("poolCustomers", 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Tworzenie kolejki komunikatów
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    if(pthread_create(&monitoringClients, NULL, MonitorClients, "") != 0) {
        perror("pthread_create");
        exit(1);
    }

    while (true) {

    }



//    sleep(5);
//    kill(cashier, 34);
//    kill(lifeguardRec, 34);
//    kill(lifeguardKid, 34);
//    kill(lifeguardOly, 34);

//    sleep(10);


//    kill(lifeguardOly, 35);

//    kill(lifeguardOly, 34);
    return 0;
}

void *MonitorClients() {

    while (1) {
        if (msgrcv(msgid, &mgrMsg, sizeof(mgrMsg) - sizeof(long), MANAGER_CHANNEL, 0) == -1) {
            if (errno == EINTR) continue; // Przerwanie przez sygnał
            perror("msgrcv");
            exit(1);
        }

        if (mgrMsg.action == 1) {
            AddClient(mgrMsg.pid); // Dodaj klienta
        } else if (mgrMsg.action == 0) {
            RemoveClient(mgrMsg.pid); // Usuń klienta
        }
    }
}


void AddClient(pid_t pid) {
    clientPIDs[numberOfClients] = pid;
    numberOfClients ++;
    printf("Dodano klienta PID %d. Liczba klientów: %d\n", pid, numberOfClients);
}

void RemoveClient(pid_t pid) {
    for (int i = 0; i < numberOfClients; i++) {
        if (clientPIDs[i] == pid) {
            // Przesunięcie elementów w tablicy, aby wypełnić lukę
            for (int j = i; j < numberOfClients - 1; j++) {
                clientPIDs[j] = clientPIDs[j + 1];
            }
            numberOfClients--;
            printf("Usunięto klienta PID %d. Liczba klientów: %d\n", pid, numberOfClients);
            return;
        }
    }
    printf("Nie znaleziono klienta PID %d na liście.\n", pid);

}

