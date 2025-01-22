#include "common.h"



void SetUpLifeguard(char *code);
void SetUpIPC();
void* ClientIn();
void* ClientOut();
void PrintPoolPids();
void ClosePoolHandler(int);
void OpenPoolHandler(int);

int poolChannelEnter, poolChannelExit, poolSize;
int shKey, shmid;
int msgid, msgKey;
struct PoolStruct *pool;
struct LifeguardMessage msg;
pthread_t lifegaurdInThread, lifegaurdOutThread;

sem_t shSem, poolSem;

int main(int argc, char *argv[]) {

    signal(34, ClosePoolHandler);
    signal(35, OpenPoolHandler);

    if (argc < 2) {
        printf("Zła liczba argumentów ratownika\n");
        exit(1);
    }

    SetUpLifeguard(argv[1]);
    SetUpIPC();

    sem_init(&shSem, 0 , 1);
    sem_init(&poolSem, 0, 1);

    if (pthread_create(&lifegaurdInThread, NULL, ClientIn, "") != 0) {
        perror("pthread_create");
        exit(1);
    }

    if (pthread_create(&lifegaurdOutThread, NULL, ClientOut, "") != 0) {
        perror("pthread_create");
        exit(1);
    }

    pthread_join(lifegaurdInThread, NULL);
    pthread_join(lifegaurdOutThread, NULL);


    printf("Wszystkie watki zakonczyly dzialanie\n");

    // Usunięcie semaforów
    sem_destroy(&shSem);
    sem_destroy(&poolSem);

    // Odłączenie pamięci dzielonej
    if (shmdt(pool) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Usuwanie pamięci dzielonej i kolejki komunikatów
    shmctl(shmid, IPC_RMID, NULL);
    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}


void SetUpLifeguard(char *code) {



    int poolCode = atoi(code);

    switch (poolCode) {
        case RECREATIONAL_POOL_CODE:
            poolSize = RECREATIONAL_POOL_SIZE;
            poolChannelEnter = RECREATIONAL_ENTER_CHANNEL;
            poolChannelExit = RECREATIONAL_EXIT_CHANNEL;
            printf("Stworzono ratownika b.rekreacyjnego\n");
            break;
        case KIDS_POOL_CODE:
            poolSize = KIDS_POOL_SIZE;
            poolChannelEnter = KIDS_ENTER_CHANNEL;
            poolChannelExit = KIDS_EXIT_CHANNEL;
            printf("Stworzono ratownika b.brodzik %d\n",getpid());
            break;
        case OLYMPIC_POOL_CODE:
            poolSize = OLYMPIC_POOL_SIZE;
            poolChannelEnter = OLYMPIC_ENTER_CHANNEL;
            poolChannelExit = OLYMPIC_EXIT_CHANNEL;
            printf("Stworzono ratownika b.olimpijskiego\n");
            break;
        default:
            fprintf(stderr, "Niepoprawny typ basenu: %s\n", code);
            exit(1);
    }




}

void SetUpIPC() {


    //-------------------Tworzenie pamieci dzielonej basenu

    if (poolChannelEnter == RECREATIONAL_ENTER_CHANNEL) {
        shKey = ftok("poolRec", 65);
    }
    else if (poolChannelEnter == KIDS_ENTER_CHANNEL) {
        shKey = ftok("poolKid", 65);
    }
    else {
        shKey = ftok("poolOly", 65);
    }

    if (shKey == -1) {
        perror("ftok");
        exit(1);
    }

    // Tworzenie segmentu pamięci dzielonej
    shmid = shmget(shKey, sizeof(struct PoolStruct) + poolSize * sizeof(pid_t), 0600 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget rat");
        exit(1);
    }

    // Dołączanie pamięci dzielonej
    pool = (struct PoolStruct *)shmat(shmid, NULL, 0);
    if (pool == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Inicjalizacja danych basenu
    pool->client_count = 0;
    pool->total_age = 0;
    for (int i = 0; i < poolSize; i++)
    {
        pool->pids[i] = 0;
    }

    //printf("Stworzono pam dzielona dla basenu: %s\n", argv[1]);

    //-------------------Tworzenie kolejki komunikatow

    msgKey = ftok("queuefile", 65);
    if (msgKey == -1) {
        perror("ftok");
        exit(1);
    }

    msgid = msgget(msgKey, 0600 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Ratownik gotowy do pracy.\n");


}

void* ClientIn() {

    int semValue;

    while (1) {

        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelEnter, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        sem_wait(&shSem);

        printf("[RAT %d]Otrzymano zapytanie od klienta PID: %d, wiek: %d, d.wiek: %d\n", poolChannelEnter,msg.pid, msg.age, msg.kidAge);

        int new_count = pool->client_count + 1;
        int new_total_age = pool->total_age + msg.age;
        if (msg.hasKid) {
            new_count = new_count + 1;
            new_total_age = new_total_age + msg.kidAge;
        }
        int new_avg_age = (new_count > 0) ? (new_total_age / new_count) : 0;
        bool enterFlag = true;
        char reason[50];

        // Sprawdzanie warunków wejścia do basenu
        //TODO sprawdzic czy dziala
        sem_getvalue(&poolSem, &semValue);
        if (semValue == 0) {
            enterFlag = false;
            strcpy(reason, "basen jest zamkniety.\n");
        }
        else if (poolChannelEnter == OLYMPIC_ENTER_CHANNEL && (msg.age < 18 || msg.hasKid)) {
            enterFlag = false;
            strcpy(reason, "nie masz 18lat na b.olimp.\n");
        } else if (poolChannelEnter == KIDS_ENTER_CHANNEL && msg.kidAge > 5) {
            enterFlag = false;
            strcpy(reason, "twoje dzicko jest za duze na brodzik.\n");
        } else if (poolChannelEnter == KIDS_ENTER_CHANNEL && msg.hasKid == false)
        {
            enterFlag = false;
            strcpy(reason, "nie masz dzicka do brodzika.\n");
        } else if (poolChannelEnter == RECREATIONAL_ENTER_CHANNEL && new_avg_age > MAX_AGE)
        {
            enterFlag = false;
            strcpy(reason, "przekroczono średni wiek.\n");
        } else if (new_count > poolSize) {
            enterFlag = false;
            strcpy(reason, "osiągnieto limit osób w basenie.\n");
        }

        if (enterFlag) {
            // Klient może wejść do basenu
            pool->pids[pool->client_count] = msg.pid;
            pool->client_count++;
            pool->total_age += msg.age;
            if (msg.hasKid){
                pool->pids[pool->client_count] = 0;
                pool->client_count++;
                pool->total_age += msg.kidAge;
            }

            msg.allowed = 1;
            printf("[RAT %d]Klient PID: %d wpuszczony do basenu %d.\n", poolChannelEnter,msg.pid, poolChannelEnter);
        } else {
            // Klient nie może wejść do basenu
            msg.allowed = 0;
            printf("[RAT %d]Klient PID: %d nie został wpuszczony do basenu %d. Powód: \n%s", poolChannelEnter,msg.pid, poolChannelEnter, reason);
        }

        sem_post(&shSem);

        // Wysyłanie odpowiedzi do klienta
        msg.mtype = msg.pid; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        PrintPoolPids();
    }


    printf("KONIEC WATKUKONIEC WATKUKONIEC WATKUKONIEC \n");

}

void* ClientOut() {
    int client_pid, found, client_age;


    while (1) {
        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelExit, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        sem_wait(&shSem);
        found = 0;
        client_pid = msg.pid;
        client_age = msg.age;

        printf("[RAT %d]Otrzymano zapytanie o wyjście od klienta PID: %d, wiek: %d\n", poolChannelExit,msg.pid, msg.age);

        // Szukanie klienta w tablicy PID
        for (int i = 0; i < pool->client_count; i++) {
            if (pool->pids[i] == client_pid) {
                found = 1;

                pool->pids[i] = 0;

                int shift = msg.hasKid ? 2 : 1;

                // Usunięcie klienta przez przesunięcie elementów w tablicy
                for (int j = i; j < pool->client_count - shift; j++) {
                    pool->pids[j] = pool->pids[j + shift];
                }

                // Aktualizacja danych basenu
                pool->client_count-=shift;
                pool->total_age -= msg.hasKid ? client_age+msg.kidAge : client_age;

                break;
            }
        }

        sem_post(&shSem);

        if (found) {

            printf("[RAT %d]Klient PID: %d dostal pozwolenie na opuszczenie basenu.\n", poolChannelExit,client_pid);
            msg.allowed = 1;
        }
        else
        {
            printf("[RAT %d]Klient PID: %d nie został znaleziony w basenie.\n", poolChannelExit,client_pid);
            msg.allowed = 0;
        }
        // Wysyłanie odpowiedzi do klienta
        msg.mtype = msg.pid; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        PrintPoolPids();
    }

    printf("KONIEC WATKUKONIEC WATKUKONIEC WATKUKONIEC \n");
}

void PrintPoolPids() {
    printf("\033[4;39;49m[RAT %d]Lista PID klientów w basenie:\033[0m\n", poolChannelEnter);
    for (int i = 0; i < pool->client_count; i++) {
        printf("Klient %d: PID = %d\n", i + 1, pool->pids[i]);
    }
}

void ClosePoolHandler(int sig) {
    printf("\033[1;31;40m-------------[RAT %d] Zamknięcie basenu-------------\033[0m\n", poolChannelEnter);

    sem_wait(&poolSem);
    for (int i = 0; i < pool->client_count; i++) {
        if (pool->pids[i] != 0) {
            kill(pool->pids[i], EXIT_POOL_SIGNAL);
        }
    }
    signal(34, ClosePoolHandler);
    signal(35, OpenPoolHandler);

    printf("Zakonczono oblsuge watku zamkniecia\n");

    return 0;
}

void OpenPoolHandler(int sig) {
    printf("\033[1;32;40m-------------[RAT %d] Otwarcie basenu-------------\033[0m\n", poolChannelEnter);
    sem_post(&poolSem); // Odblokowanie dostępu do basenu
    printf("Zakonczono oblsuge watku otwarcia\n");

    signal(34, ClosePoolHandler);
    signal(35, OpenPoolHandler);
    return 0;
}
