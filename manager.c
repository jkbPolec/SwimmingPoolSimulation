#include "common.h"


pid_t cashier, lifeguardRec, lifeguardKid, lifeguardOly;
int shmid;
struct PoolStatus *poolStatus;
pid_t clientPIDs[1000];



int shID, shKey;
int semID;

void SetupMemory();
void SetClosingTime();
void SetupSemaphore();
int CheckSemaphoreValue();

int main(int argc, char* argv[]) {


    if (argc != 5){
        fprintf(stderr,"Niepoprawna liczba argumentow!\n");
        exit(1);
    }
    cashier = (pid_t)atoi(argv[1]);
    lifeguardRec = (pid_t)atoi(argv[2]);
    lifeguardKid = (pid_t)atoi(argv[3]);
    lifeguardOly = (pid_t)atoi(argv[4]);

    SetupMemory();
    SetupSemaphore();

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


//    sleep(2);
//    poolStatus->isOpened = false;
//    kill(cashier, CASHIER_SIGNAL);
//    kill(lifeguardRec, CLOSE_POOL_SIGNAL);
//    kill(lifeguardKid, CLOSE_POOL_SIGNAL);
//    kill(lifeguardOly, CLOSE_POOL_SIGNAL);

//    kill(lifeguardRec, OPEN_POOL_SIGNAL);
//    kill(lifeguardKid, OPEN_POOL_SIGNAL);
//    kill(lifeguardOly, OPEN_POOL_SIGNAL);
//
//    int result = system("killall -s 34 klient");
//    if (result == -1) {
//        perror("system");
//    } else {
//        printf("Wysłano sygnał 34 do wszystkich procesów 'klient'.\n");
//    }


    while (CheckSemaphoreValue() != 0) {

    }

    printf("Wszyscy wyszli\n");
    CheckSemaphoreValue();
    CheckSemaphoreValue();
    CheckSemaphoreValue();


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

    //Funkcja tworzy semafor, potrzebny do monitorowania ilosci klientow na basenie
void SetupSemaphore() {

    // Tworzenie semafora
    semID = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        perror("semget");
        exit(1);
    }

    // Inicjalizacja semafora na 0
    if (semctl(semID, 0, SETVAL, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    printf("Semafor utworzony i zainicjalizowany na 0.\n");
}

    //Funkcja sprawdza, ile klientow jest na basenie
int CheckSemaphoreValue() {
    int semValue = semctl(semID, 0, GETVAL);
    if (semValue == -1) {
        perror("semctl getval");
        exit(1);
    }
    //printf("[MAN]Aktualna wartość semafora: %d\n", semValue);
    return semValue;
}


    //Funkcja tworzy pamiec dzielona, klienci beda sprawdzac
    //czy basen jest otwarty czy zamkniety
void SetupMemory() {
    // Generowanie klucza do pamieci dzielonej
    shKey = ftok("poolStatus", 65);
    if (shKey == -1) {
        perror("ftok");
        exit(1);
    }

    // Tworzenie pamieci dzielonej
    shID = shmget(shKey, sizeof(struct PoolStatus), 0600 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget man");
        exit(1);
    }

    // Dołączanie pamięci dzielonej
    poolStatus = (struct PoolStatus *)shmat(shID, NULL, 0);
    if (poolStatus == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    poolStatus->isOpened = true;
}


// Funkcja czyszcząca semafor
void CleanupSemaphore() {
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror("semctl remove");
        exit(1);
    }
    printf("Semafor usunięty.\n");
}

