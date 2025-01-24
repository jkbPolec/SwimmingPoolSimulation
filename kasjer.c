#include "common.h"


pthread_t queueThread;
sem_t sem;
key_t key;
int msgid;
struct message msg;
sem_t queueSem;


void TogglePool(int);
void *QueueThread(void *arg);

int main() {

    signal(CASHIER_SIGNAL, TogglePool);
    sem_init(&queueSem, 0, 1);

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

    while (true)
    {

    }


    return 0;
}

void *QueueThread(void* arg) {

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
        //printf("Kasjer wysłał odpowiedź do klienta o PID: %ld\n", msg.mtype);

        if (msg.kidAge < 10 && msg.allowed == 1) {
            //printf("Dziecko nie placi za wejscie, klient o PID: %ld\n", msg.mtype);
        }
    }


}


void TogglePool(int sig) {
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