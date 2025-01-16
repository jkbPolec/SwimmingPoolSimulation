#include "common.h"

struct clientData {
    int age;
    bool isVIP;
};

void queueRoutine() {
    int key, msgid;
    struct message msg;

    // Generowanie klucza do kolejki
    key = ftok("queuefile", 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Otwieranie kolejki komunikatów
    msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    // Wysyłanie komunikatu z PID-em klienta do kasjera
    msg.mtype = 1;  // Typ komunikatu (1 oznacza komunikat do kasjera)
    msg.pid = getpid(); // PID klienta

    if (msgsnd(msgid, &msg, sizeof(msg.pid), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    printf("Klient (PID: %d) wysłał swój PID do kasjera.\n", msg.pid);
    msg.mtype = msg.pid;

    // Oczekiwanie na odpowiedź od kasjera
    if (msgrcv(msgid, &msg, sizeof(msg.pid), msg.pid, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    printf("Klient (PID: %d) otrzymał odpowiedź od kasjera.\n", msg.pid);


}


int main() {
    key_t key;
    int msgid, chosenPool;
    struct message msg;
    struct clientData clientData;
    pid_t myPid;

    myPid = getpid();
    srand(time(NULL) + myPid);
    clientData.age = rand() % 70 + 1;
    int VIP = rand() % 100 + 1;
    if (VIP == 1) {clientData.isVIP = true;}

    chosenPool = rand() % 3 + 1;

    if (VIP != VIP_CODE) {
        queueRoutine();
    }




    return 0;
}
