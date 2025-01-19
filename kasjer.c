#include "common.h"


pthread_t queueThread;
sem_t sem;
key_t key;
int msgid;
struct message msg;


void TestHandler(int);
void *QueueThread(void *arg);

int main() {

    signal(34, TestHandler);


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

    while (1) {
        // Odbieranie wiadomości od klienta
        errno = 0;
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

        printf("Kasjer odebrał PID klienta: %d\n", msg.pid);
        //sleep(1);
        // Odsyłanie klientowi wiadomosci, na jego PID
        msg.mtype = msg.pid;

        // Wysyłanie odpowiedzi do klienta
        if (msgsnd(msgid, &msg, sizeof(msg.pid), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("Kasjer wysłał odpowiedź do klienta o PID: %ld\n", msg.mtype);
    }


}


void TestHandler(int sig) {
    printf("Kasjer zglasza sie!\n");
}