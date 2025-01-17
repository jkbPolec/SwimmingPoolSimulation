#include "common.h"

#define MAX_PEOPLE 10
#define MAX_AGE_AVG 40
#define SHM_KEY 1234
#define MSG_KEY 5678



void setUp(int argc, char *argv[], int* poolSize, int* poolChannel);

int main(int argc, char *argv[]) {

    int shmid, shKey;
    struct PoolStruct *pool;
    int poolSize, poolChannel;
    int msgid;
    struct LifeguardMessage msg;

    setUp(argc, argv, &poolSize, &poolChannel);


    //-------------------Tworzenie pamieci dzielonej basenu

    shKey = ftok("shmfile", 65);
    if (shKey == -1) {
        perror("ftok");
        exit(1);
    }

    // Tworzenie segmentu pamięci dzielonej
    shmid = shmget(SHM_KEY, sizeof(struct PoolStruct) + poolSize * sizeof(pid_t), 0600 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
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

    msgid = msgget(MSG_KEY, 0600 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Ratownik gotowy do pracy.\n");


    while (1) {
        // Odbieranie zapytania od klienta
        if (msgrcv(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), poolChannel, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Otrzymano zapytanie od klienta PID: %ld, wiek: %d\n", msg.mtype, msg.age);

        // Sprawdzanie warunków wejścia do basenu
        int new_count = pool->client_count + 1;
        int new_total_age = pool->total_age + msg.age;
        int new_avg_age = (new_count > 0) ? (new_total_age / new_count) : 0;

        if (new_count <= MAX_PEOPLE && new_avg_age <= MAX_AGE_AVG) {
            // Klient może wejść do basenu
            pool->client_count++;
            pool->total_age += msg.age;
            msg.allowed = 1;
            printf("Klient PID: %ld wpuszczony do basenu.\n", msg.mtype);
        } else {
            // Klient nie może wejść do basenu
            msg.allowed = 0;
            printf("Klient PID: %ld nie został wpuszczony. Powód: ", msg.mtype);
            if (new_count > MAX_PEOPLE) {
                printf("osiągnięto limit osób.\n");
            } else {
                printf("przekroczono średni wiek.\n");
            }
        }

        // Wysyłanie odpowiedzi do klienta
        msg.mtype = msg.mtype; // Typ wiadomości odpowiada PID klienta
        if (msgsnd(msgid, &msg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }

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


void setUp(int argc, char *argv[], int* poolSize, int* poolChannel) {

    if (argc < 2) {
        printf("Zła liczba argumentów ratownika\n");
        exit(1);
    }

    int poolCode = atoi(argv[1]);

    switch (poolCode) {
        case RECREATIONAL_POOL_CODE:
            *poolSize = RECREATIONAL_POOL_SIZE;
            *poolChannel = RECREATIONAL_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.rekreacyjnego\n");
            break;
        case KIDS_POOL_CODE:
            *poolSize = KIDS_POOL_SIZE;
            *poolChannel = KIDS_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.brodzik\n");
            break;
        case OLYMPIC_POOL_CODE:
            *poolSize = OLYMPIC_POOL_SIZE;
            *poolChannel = OLYMPIC_LIFEGAURD_CHANNEL;
            printf("Stworzono ratownika b.olimpijskiego\n");
            break;
        default:
            fprintf(stderr, "Niepoprawny typ basenu: %s\n", argv[1]);
            exit(1);
    }


}