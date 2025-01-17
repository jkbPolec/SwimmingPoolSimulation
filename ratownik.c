#include "common.h"



void SetUpLifeguard(char *code);
void SetUpIPC();
void* ClientIn();
void* ClientOut();
void PrintPoolPids();

int poolChannelEnter, poolChannelExit, poolSize;
int shKey, shmid;
int msgid, msgKey;
struct PoolStruct *pool;
struct LifeguardMessage msg;
pthread_t lifegaurdInThread, lifegaurdOutThread;



int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Zła liczba argumentów ratownika\n");
        exit(1);
    }

    SetUpLifeguard(argv[1]);
    SetUpIPC();

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
            printf("Stworzono ratownika b.brodzik\n");
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

    if (poolChannelEnter == RECREATIONAL_POOL_CODE) {
        shKey = ftok("poolRec", 65);
    }
    else if (poolChannelEnter == KIDS_POOL_CODE) {
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

    while (1) {
        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelEnter, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Otrzymano zapytanie od klienta PID: %d, wiek: %d\n", msg.pid, msg.age);

        // Sprawdzanie warunków wejścia do basenu
        int new_count = pool->client_count + 1;
        int new_total_age = pool->total_age + msg.age;
        int new_avg_age = (new_count > 0) ? (new_total_age / new_count) : 0;
        bool ageFlag = true;

        if (poolChannelEnter == OLYMPIC_ENTER_CHANNEL && msg.age < 18) {
            ageFlag = false;
        }

        if (new_count <= poolSize && new_avg_age <= MAX_AGE && ageFlag) {
            // Klient może wejść do basenu
            pool->pids[pool->client_count] = msg.pid;
            pool->client_count++;
            pool->total_age += msg.age;

            msg.allowed = 1;
            printf("Klient PID: %d wpuszczony do basenu %d.\n", msg.pid, poolChannelEnter);
        } else {
            // Klient nie może wejść do basenu
            msg.allowed = 0;
            printf("Klient PID: %d nie został wpuszczony do basenu %d. Powód: ", msg.pid, poolChannelEnter);
            if (!ageFlag) {
                printf("nie jestes dorosly.\n");
            } else if (new_count > poolSize) {
                printf("osiągnięto limit osób.\n");
            } else {
                printf("przekroczono średni wiek.\n");
            }
        }

        // Wysyłanie odpowiedzi do klienta
        msg.mtype = msg.pid; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        PrintPoolPids();
    }

}
void* ClientOut() {
    int client_pid, found, client_age;
    found = 0;

    while (1) {
        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelExit, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        client_pid = msg.pid;
        client_age = msg.age;

        printf("Otrzymano zapytanie o wyjście od klienta PID: %d, wiek: %d\n", msg.pid, msg.age);

        // Szukanie klienta w tablicy PID
        for (int i = 0; i < pool->client_count; i++) {
            if (pool->pids[i] == client_pid) {
                found = 1;

                // Usunięcie klienta przez przesunięcie elementów w tablicy
                for (int j = i; j < pool->client_count - 1; j++) {
                    pool->pids[j] = pool->pids[j + 1];
                }

                // Aktualizacja danych basenu
                pool->client_count--;
                pool->total_age -= client_age;

                printf("Klient PID: %d opuścił basen.\n", client_pid);
                break;
            }
        }

        if (found) {

            printf("Klient PID: %d dostal pozwolenie na opuszczenie basenu.\n", client_pid);
            msg.allowed = 1;
        }
        else
        {
            printf("Klient PID: %d nie został znaleziony w basenie.\n", client_pid);
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
}


void PrintPoolPids() {
    printf("Lista PID klientów w basenie:\n");
    for (int i = 0; i < pool->client_count; i++) {
        printf("Klient %d: PID = %d\n", i + 1, pool->pids[i]);
    }
}
