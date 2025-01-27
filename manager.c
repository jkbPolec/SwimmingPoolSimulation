#include "common.h"


pid_t cashier, lifeguardRec, lifeguardKid, lifeguardOly;
int shmid;
struct PoolStatus *poolStatus;
pid_t clientPIDs[1000];

pthread_t timeThread;


int shID, shKey;
int semID;

void SetupMemory();
void SetClosingTime();
void SetupSemaphore();
int CheckSemaphoreValue();
void *CheckTime(void *arg);
void CleanupResources();
void SigintHandler(int);

int main(int argc, char* argv[]) {

    signal(SIGINT, SigintHandler);


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

    // Tworzenie wątku
    if (pthread_create(&timeThread, NULL, CheckTime, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }


    pthread_join(timeThread, NULL);

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

        // Sprawdzenie i ewentualne utworzenie pliku "queuefile"
        FILE *file = fopen("poolStatus", "a"); // "a" otwiera plik w trybie dopisania lub tworzy go, jeśli nie istnieje
        if (file == NULL) {
            perror("fopen");
            exit(1);
        }
        fclose(file);


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


void *CheckTime(void *arg) {
    while (1) {
        // Pobieranie aktualnego czasu
        time_t now = time(NULL);
        struct tm *local = localtime(&now);

        // Sprawdzanie godzin i minut
        if (local->tm_hour < 12 || (local->tm_hour == 12 && local->tm_min == 0)) {
            printf("Jest za wczesnie na otwarcie basenu\n");
            break;
        } else if (local->tm_hour > 23 || (local->tm_hour == 23 && local->tm_min > 58)) {
            printf("Przyszla pora na zamkniecie basenu\n");
            break;
        }
        sleep(1);
    }
    return NULL;
}


// Funkcja czyszcząca semafor
void CleanupSemaphore() {
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror("semctl remove");
        exit(1);
    }
    printf("Semafor usunięty.\n");
}


void ClosePools() {
    kill(cashier, CASHIER_SIGNAL);

    int result = system("killall -s 34 klient");
    if (result == -1) {
        perror("system");
    } else {
        printf("Wysłano sygnał 34 do wszystkich procesów 'klient'.\n");
    }

    while (CheckSemaphoreValue() != 0) {

    }

    printf("Wszyscy wyszli\n");
}


void CleanupResources() {
    printf("\033[1;33;40mUsuwanie zasobów...\033[0m\n");

    // Odłączanie pamięci dzielonej
    if (shmdt(poolStatus) == -1) {
        perror("shmdt");
    }

    // Usuwanie pamięci dzielonej
    if (shmctl(shID, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    } else {
        printf("Pamięć dzielona usunięta.\n");
    }

    // Usuwanie semafora
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror("semctl remove");
    } else {
        printf("Semafor usunięty.\n");
    }

    exit(0);
}


void SigintHandler(int sig) {
    printf("\033[1;31;40mOtrzymano SIGINT [MAN] (Ctrl+C). Zamykanie zasobów...\033[0m\n");
    CleanupResources();
}
