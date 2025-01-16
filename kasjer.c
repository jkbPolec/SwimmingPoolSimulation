#include "common.h"


int main() {
    key_t key;
    int msgid;
    struct message msg;

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

    printf("Kasjer gotowy do odbierania wiadomości...\n");

    while (1) {
        // Odbieranie wiadomości od klienta
        if (msgrcv(msgid, &msg, sizeof(msg.pid), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("Kasjer odebrał PID klienta: %d\n", msg.pid);

        // Przygotowanie odpowiedzi do klienta
        msg.mtype = msg.pid;  // Typ komunikatu to PID klienta
        msg.pid = msg.pid;  // Typ komunikatu to PID klienta

        // Wysyłanie odpowiedzi do klienta
        if (msgsnd(msgid, &msg, sizeof(msg.pid), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("Kasjer wysłał odpowiedź do klienta o PID: %ld\n", msg.mtype);
    }

    return 0;
}
