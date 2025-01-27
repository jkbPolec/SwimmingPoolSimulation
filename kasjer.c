#include "common.h"


pthread_t queueThread;
sem_t sem;
key_t key;
int msgid;
struct message msg;
sem_t queueSem;


void TogglePool();
void *QueueThread();
void CleanupResources();
void SigintHandler();

int main() {

    signal(SIGINT, SigintHandler);
    signal(CASHIER_SIGNAL, TogglePool);

    sem_init(&queueSem, 0, 1);


    // Sprawdzenie i ewentualne utworzenie pliku "queuefile"
    FILE *file = fopen("queuefile", "a"); // "a" otwiera plik w trybie dopisania lub tworzy go, jeśli nie istnieje
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }
    fclose(file);


    // Generowanie klucza do kolejki
    key = ftok("queuefile", 65);
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

    if (pthread_create(&queueThread, NULL, QueueThread, "") != 0) {
        perror("pthread_create");
        exit(1);
    }

    pthread_join(queueThread, NULL);


    return 0;
}

/**
 * @brief Wątek obsługujący komunikację w kolejce.
 *
 * @note Obsługuje odbieranie i wysyłanie wiadomości klientów w kolejce.
 */
void *QueueThread() {

    printf("Kasjer gotowy do odbierania wiadomości...\n");
    int val;
    while (1) {
        // Odbieranie wiadomości od klienta
        errno = 0;
        val = 0;

        if(msgrcv(msgid, &msg, sizeof(msg.pid), CASHIER_CHANNEL, 0) == -1) {
            if (errno == EINTR) {
                printf("msgrcv przerwane przez sygnał, wznowiono...\n");
                continue;
            }
            else {
                perror("msgrcv");
                exit(1);
            }
        }

        //printf("Kasjer odebrał PID klienta: %d\n", msg.pid);
        //sleep(1);
        // Odsyłanie klientowi wiadomosci, na jego PID
        msg.mtype = msg.pid;
        sem_getvalue(&queueSem, &val);



        if (val == 1) {
            msg.allowed = 1;
        } else {
            msg.allowed = 0;
        }



        // Wysyłanie odpowiedzi do klienta
        if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("\033[1;35;49mKasjer wysłał odpowiedź do klienta o PID: %ld\033[0m\n", msg.mtype);

        if (msg.kidAge < 10 && msg.allowed == 1) {
            //printf("Dziecko nie placi za wejscie, klient o PID: %ld\n", msg.mtype);
        }
    }

    return NULL;
}

/**
 * @brief Obsługuje sygnał otwierania/zamykania kolejki.
 *
 * @param sig Numer sygnału.
 * @note Zmienia wartość semafora `queueSem`.
 */
void TogglePool() {
    int val;

    sem_getvalue(&queueSem, &val);
    if (val == 1) {
        printf("\033[1;31;40m-------------Kasjer zamyka kolejke-------------\033[0m\n\n");
        sem_wait(&queueSem);
    }
    else {
        printf("\033[1;32;40m-------------Kasjer otwiera kolejke-------------\033[0m\n\n");
        sem_post(&queueSem);
    }

    signal(CASHIER_SIGNAL, TogglePool);

}

// Funkcja do usuwania semaforów i kolejki komunikatów

/**
 * @brief Czyści zasoby używane przez program.
 *
 * @note Usuwa kolejkę komunikatów i semafory.
 */
void CleanupResources() {
    printf("\n\033[1;33;40mUsuwanie zasobów...\033[0m\n");

    // Usuwanie kolejki komunikatów
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl (IPC_RMID)");
    } else {
        printf("Kolejka komunikatów usunięta.\n");
    }

    // Usuwanie semafora
    if (sem_destroy(&queueSem) == -1) {
        perror("sem_destroy");
    } else {
        printf("Semafor usunięty.\n");
    }

    exit(0);
}

// Handler sygnału SIGINT

/**
 * @brief Obsługuje sygnał SIGINT (Ctrl+C) i zamyka zasoby.
 *
 * @param sig Numer sygnału.
 * @note Wywołuje funkcję `CleanupResources()`.
 */
void SigintHandler() {
    printf("\n\033[1;31;40mOtrzymano SIGINT [KAS] (Ctrl+C). Kończenie programu...\033[0m\n");
    CleanupResources();
}