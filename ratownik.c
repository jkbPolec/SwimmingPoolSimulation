#include "common.h"
#include <sys/prctl.h>

void SetUpLifeguard(char *code);
void SetUpIPC();
void* ClientIn();
void* ClientOut();
void PrintPoolPids(int sig);
void ClosePoolHandler(int);
void OpenPoolHandler(int);

int poolChannelEnter, poolChannelExit, poolSize;
int shKey, shmid;
int msgid, msgKey;
struct PoolStruct *pool;
struct LifeguardMessage msgIn, msgOut;
pthread_t lifegaurdInThread, lifegaurdOutThread;

sem_t poolSem;

pthread_mutex_t mutex;

int main(int argc, char *argv[]) {

    signal(CLOSE_POOL_SIGNAL, ClosePoolHandler);
    signal(OPEN_POOL_SIGNAL, OpenPoolHandler);
    signal(PRINT_POOL_SIGNAL, PrintPoolPids);

    pthread_mutex_init(&mutex, NULL);


    if (argc < 2) {
        printf("Zła liczba argumentów ratownika\n");
        exit(1);
    }

    SetUpLifeguard(argv[1]);
    SetUpIPC();

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
            if (prctl(PR_SET_NAME, "ratownik R", 0, 0, 0) == -1) {
                perror("prctl");
                exit(1);
            }
            printf("Stworzono ratownika b.rekreacyjnego\n");
            break;
        case KIDS_POOL_CODE:
            poolSize = KIDS_POOL_SIZE;
            poolChannelEnter = KIDS_ENTER_CHANNEL;
            poolChannelExit = KIDS_EXIT_CHANNEL;
            if (prctl(PR_SET_NAME, "ratownik B", 0, 0, 0) == -1) {
                perror("prctl");
                exit(1);
            }
            printf("Stworzono ratownika b.brodzik\n");
            break;
        case OLYMPIC_POOL_CODE:
            poolSize = OLYMPIC_POOL_SIZE;
            poolChannelEnter = OLYMPIC_ENTER_CHANNEL;
            poolChannelExit = OLYMPIC_EXIT_CHANNEL;
            if (prctl(PR_SET_NAME, "ratownik O", 0, 0, 0) == -1) {
                perror("prctl");
                exit(1);
            }
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
        while (1) {
            if (msgrcv(msgid, &msgIn, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelEnter, 0) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("msgrcv");
                    exit(1);
                }
            }
            break;
        }


        pthread_mutex_lock(&mutex);

        //printf("[RAT %d]Otrzymano zapytanie od klienta PID: %d, wiek: %d, d.wiek: %d\n", poolChannelEnter,msgIn.pid, msgIn.age, msgIn.kidAge);

        int new_count = pool->client_count + 1;
        int new_total_age = pool->total_age + msgIn.age;
        if (msgIn.hasKid) {
            new_count = new_count + 1;
            new_total_age = new_total_age + msgIn.kidAge;
        }
        int new_avg_age = (new_count > 0) ? (new_total_age / new_count) : 0;
        bool enterFlag = true;
        char reason[50];

        // Sprawdzanie warunków wejścia do basenu
        sem_getvalue(&poolSem, &semValue);
        if (semValue == 0) {
            enterFlag = false;
            strcpy(reason, "basen jest zamkniety.\n");
        }
        else if (poolChannelEnter == OLYMPIC_ENTER_CHANNEL && (msgIn.age < 18 || msgIn.hasKid)) {
            enterFlag = false;
            strcpy(reason, "nie masz 18lat na b.olimp.\n");
        } else if (poolChannelEnter == KIDS_ENTER_CHANNEL && msgIn.kidAge > 5) {
            enterFlag = false;
            strcpy(reason, "twoje dzicko jest za duze na brodzik.\n");
        } else if (poolChannelEnter == KIDS_ENTER_CHANNEL && msgIn.hasKid == false)
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
            pool->pids[pool->client_count] = msgIn.pid;
            pool->client_count++;
            pool->total_age += msgIn.age;
            if (msgIn.hasKid){
                pool->pids[pool->client_count] = 0;
                pool->client_count++;
                pool->total_age += msgIn.kidAge;
            }

            msgIn.allowed = 1;
            printf("[RAT %d]Klient PID: %d wpuszczony do basenu %d.\n", poolChannelEnter,msgIn.pid, poolChannelEnter);
        } else {
            msgIn.allowed = 0;
            printf("[RAT %d]Klient PID: %d nie został wpuszczony do basenu %d. Powód: \n%s", poolChannelEnter,msgIn.pid, poolChannelEnter, reason);
        }

        // Wysyłanie odpowiedzi do klienta
        msgIn.mtype = msgIn.pid; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msgIn, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        pthread_mutex_unlock(&mutex);

    }


    printf("KONIEC WATKUKONIEC WATKUKONIEC WATKUKONIEC \n");

}

void* ClientOut() {
    int client_pid, found, client_age;

    while (1) {
        // Odbieranie zapytania od klienta
        while (1) {
            if (msgrcv(msgid, &msgOut, sizeof(struct LifeguardMessage) - sizeof(long), poolChannelExit, 0) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("msgrcv");
                    exit(1);
                }
            }
            break;
        }


        //printf("[RAT %d]Otrzymano zapytanie o wyjście od klienta PID: %d, wiek: %d\n", poolChannelExit,msgOut.pid, msgOut.age);

        found = 0;
        client_pid = msgOut.pid;
        client_age = msgOut.age;
        pthread_mutex_lock(&mutex);

        // Szukanie klienta w tablicy PID
        for (int i = 0; i < pool->client_count; i++) {
            if (pool->pids[i] == client_pid) {
                found = 1;

                pool->pids[i] = 0;

                int shift = msgOut.hasKid ? 2 : 1;

                // Usunięcie klienta przez przesunięcie elementów w tablicy
                for (int j = i; j < pool->client_count - shift; j++) {
                    pool->pids[j] = pool->pids[j + shift];
                }

                // Aktualizacja danych basenu
                pool->client_count-=shift;
                pool->total_age -= msgOut.hasKid ? client_age+msgOut.kidAge : client_age;

                break;
            }
        }


        if (found) {

            printf("[RAT %d]Klient PID: %d dostal pozwolenie na opuszczenie basenu.\n", poolChannelExit,client_pid);
            msgOut.allowed = 1;
        }
        else
        {
            printf("[RAT %d]Klient PID: %d nie został znaleziony w basenie.\n", poolChannelExit,client_pid);
            msgOut.allowed = 0;
        }

        // Wysyłanie odpowiedzi do klienta
        msgOut.mtype = msgOut.pid; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msgOut, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        pthread_mutex_unlock(&mutex);
    }

}

void PrintPoolPids(int sig) {

    printf("\033[4;39;49m[RAT %d]Lista PID klientów w basenie:\033[0m\n", poolChannelEnter);
    for (int i = 0; i < pool->client_count; i++) {
        printf("Klient %d: PID = %d\n", i + 1, pool->pids[i]);
    }

    signal(34, ClosePoolHandler);
    signal(35, OpenPoolHandler);
    signal(36, PrintPoolPids);
}

void ClosePoolHandler(int sig) {
    printf("\033[1;31;40m-------------[RAT %d] Zamknięcie basenu-------------\033[0m\n", poolChannelEnter);

    sem_wait(&poolSem);

    for (int i = 0; i < pool->client_count; i++) {
        if (pool->pids[i] != 0) {
            kill(pool->pids[i], EXIT_POOL_SIGNAL);
        }
    }


    signal(CLOSE_POOL_SIGNAL, ClosePoolHandler);
    signal(OPEN_POOL_SIGNAL, OpenPoolHandler);
    signal(PRINT_POOL_SIGNAL, PrintPoolPids);

    printf("Zakonczono oblsuge watku zamkniecia\n");

    return 0;
}

void OpenPoolHandler(int sig) {
    printf("\033[1;32;40m-------------[RAT %d] Otwarcie basenu-------------\033[0m\n", poolChannelEnter);
    sem_post(&poolSem); // Odblokowanie dostępu do basenu
    printf("Zakonczono oblsuge watku otwarcia\n");

    signal(CLOSE_POOL_SIGNAL, ClosePoolHandler);
    signal(OPEN_POOL_SIGNAL, OpenPoolHandler);
    signal(PRINT_POOL_SIGNAL, PrintPoolPids);
    return 0;
}
