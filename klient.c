#include "common.h"

struct ClientData {
    int age;
    bool isVIP;
    bool inPool;
};

void QueueRoutine();
bool EnterPool();

int enterPoolChannel,exitPoolChannel;
struct ClientData clientData;
int chosenPool;
pid_t myPid;

int main() {

    myPid = getpid();
    srand(time(NULL) + myPid);
    clientData.age = rand() % 70 + 1;
    int VIP = rand() % 100 + 1;
    if (VIP == 1) {clientData.isVIP = true;}
    clientData.inPool = false;
    chosenPool = rand() % 3 + 1;



    switch (chosenPool) {
        case 1:
            enterPoolChannel = RECREATIONAL_ENTER_CHANNEL;
            break;
        case 2:
            enterPoolChannel = KIDS_ENTER_CHANNEL;
            break;
        case 3:
            enterPoolChannel = OLYMPIC_ENTER_CHANNEL;
            break;
        default:
            fprintf(stderr,"Pool like this, doesnt exist\n");
            exit(1);
    }


    if (VIP != VIP_CODE) {
        QueueRoutine();
    }

    while(!EnterPool())
    {
        sleep(5);
    }


    return 0;
}


void QueueRoutine() {
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
    msg.mtype = CASHIER_CHANNEL;
    msg.pid = getpid();

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

bool EnterPool()
{
    struct LifeguardMessage lfgMsg;
    int lfgMsgID,  msgKey;


    //-------------------Tworzenie kolejki komunikatow do basenu
    msgKey = ftok("queuefile", 65);
    if (msgKey == -1) {
        perror("ftok");
        exit(1);
    }

    lfgMsgID = msgget(msgKey, 0600);
    if (lfgMsgID == -1) {
        perror("msgget");
        exit(1);
    }

    lfgMsg.mtype = enterPoolChannel;
    lfgMsg.pid = getpid();
    lfgMsg.age = clientData.age;
    lfgMsg.is_exit = 0;

    printf("Klient wysyła wiadomość na kanał: %ld\n", lfgMsg.mtype);



    // Wysłanie zapytania do ratownika
    if (msgsnd(lfgMsgID, &lfgMsg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    if (msgrcv(lfgMsgID, &lfgMsg, sizeof (struct  LifeguardMessage) - sizeof (long), getpid(), 0) == -1) {
        perror("msgrcv");
        exit(1);
    }


    // Wyświetlenie wyniku
    if (lfgMsg.allowed) {
        printf("Klient PID: %d wchodzi na basen %d!\n", getpid(), enterPoolChannel);
        return true;
    } else {
        printf("Klient PID: %d nie wchodzi na basen %d.\n", getpid(), enterPoolChannel);
        return false;
    }




}
