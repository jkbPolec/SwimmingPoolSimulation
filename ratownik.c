#include "common.h"



void setUpLifeguard(int argc, char *argv[]);
void setUpIPC();
void clientIn();
void clientOut();

int poolChannel, poolSize;
int shKey, shmid;
int msgid, msgKey;
struct PoolStruct *pool;
struct LifeguardMessage msg;


int main(int argc, char *argv[]) {



    setUpLifeguard(argc, argv);
    setUpIPC();

    clientIn();



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


void setUpLifeguard(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Zła liczba argumentów ratownika\n");
        exit(1);
    }

    int poolCode = atoi(argv[1]);

    switch (poolCode) {
        case RECREATIONAL_POOL_CODE:
            poolSize = RECREATIONAL_POOL_SIZE;
            poolChannel = RECREATIONAL_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.rekreacyjnego\n");
            break;
        case KIDS_POOL_CODE:
            poolSize = KIDS_POOL_SIZE;
            poolChannel = KIDS_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.brodzik\n");
            break;
        case OLYMPIC_POOL_CODE:
            poolSize = OLYMPIC_POOL_SIZE;
            poolChannel = OLYMPIC_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.olimpijskiego\n");
            break;
        default:
            fprintf(stderr, "Niepoprawny typ basenu: %s\n", argv[1]);
            exit(1);
    }




}
void setUpIPC() {


    //-------------------Tworzenie pamieci dzielonej basenu

    if (poolChannel == RECREATIONAL_POOL_CODE) {
        shKey = ftok("poolRec", 65);
    }
    else if (poolChannel == KIDS_POOL_CODE) {
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
void clientIn() {

    while (1) {
        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannel, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Otrzymano zapytanie od klienta PID: %d, wiek: %d\n", msg.pid, msg.age);

        // Sprawdzanie warunków wejścia do basenu
        int new_count = pool->client_count + 1;
        int new_total_age = pool->total_age + msg.age;
        int new_avg_age = (new_count > 0) ? (new_total_age / new_count) : 0;
        bool ageFlag = true;

        if (poolChannel == OLYMPIC_LIFEGAURD_CHANNEL && msg.age < 18) {
            ageFlag = false;
        }

        if (new_count <= poolSize && new_avg_age <= MAX_AGE && ageFlag) {
            // Klient może wejść do basenu
            pool->client_count++;
            pool->total_age += msg.age;
            msg.allowed = 1;
            printf("Klient PID: %d wpuszczony do basenu %d.\n", msg.pid, poolChannel);
        } else {
            // Klient nie może wejść do basenu
            msg.allowed = 0;
            printf("Klient PID: %d nie został wpuszczony do basenu %d. Powód: ", msg.pid, poolChannel);
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
    }

}
void clientOut() {

}
