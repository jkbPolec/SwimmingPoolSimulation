#include "common.h"

struct ClientData {
    int age;
    bool isVIP;
    bool inPool;
    bool hasKid;
};

struct ChildData {
    int age;
    //TODO pampers
    bool hasPampers;

};

void SetUpClient();
void ChoosePool();
void QueueRoutine();
bool EnterPool();
bool ExitPool();

void* ChildThread(void*);

void *MonitorTime(void*);
void TimeHandler(int sig);
void ExitPoolHandler(int sig);

int enterPoolChannel,exitPoolChannel, chosenPool;
int lfgMsgID,  msgKey;
struct LifeguardMessage lfgMsg;
struct ClientData clientData;
struct ChildData childData;
pthread_t moniotorThread, childThread;
bool leaveFlag = false;

int main() {

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);

    SetUpClient();


    if (!clientData.isVIP) {
        QueueRoutine();
    }


    time_t start_time = time(NULL);
    if(pthread_create(&moniotorThread, NULL, MonitorTime, (void*)&start_time) != 0)
    {
        perror("pthread_create monitor");
        exit(1);
    }



    /*
    while (!leaveFlag) {

        while (!clientData.inPool && !leaveFlag) {
            ChoosePool();
            clientData.inPool = EnterPool();
            sleep(5);
        }


    }
     */

    int randomNumber;
    int min = 500000;
    int max = 2500000;
    while (!leaveFlag)
    {
        ChoosePool();
        randomNumber = rand() % (max - min + 1) + min;

        while(!leaveFlag && !clientData.inPool && !(clientData.inPool = EnterPool()))
        {
            usleep(randomNumber);

            ChoosePool();
        }





        if (clientData.inPool)
        {
            usleep(randomNumber);
            ExitPool();
            usleep(randomNumber);
        }


    }


    // Czekanie na zakończenie wątku monitora czasu
    if (pthread_join(moniotorThread, NULL) != 0) {
        perror("pthread_join monitor");
    }

    // Czekanie na zakończenie wątku dziecka, jeśli istnieje
    if (clientData.hasKid) {
        if (pthread_join(childThread, NULL) != 0) {
            perror("pthread_join child");
        }
    }


    printf("Klient konczy prace\n");
    exit(0);
}

void SetUpClient() {
    srand(time(NULL) + getpid());
    //TODO dodac dziecko, zmienic zakres wieku
    clientData.age = rand() % 70 + 1;
    //clientData.age = 20;
    if (clientData.age < 10) {
        clientData.age = rand() % 41 + 30;
        clientData.hasKid = true;
        childData.age = rand() % 9 + 1;
        //TODO USUNAC
        childData.age = 3;
        if (pthread_create(&childThread, NULL, ChildThread, NULL) != 0) {
            perror("[dziecko] pthread_create");
            exit(1);
        }

    } else { clientData.hasKid = false;}


    int VIP = rand() % 100 + 1;
    if (VIP == 1) {clientData.isVIP = true;}
    clientData.inPool = false;
}

void ChoosePool() {
    chosenPool = rand() % 3 + 1;
    //TODO testy
    //chosenPool = 3;

    switch (chosenPool) {
        case 1:
            enterPoolChannel = RECREATIONAL_ENTER_CHANNEL;
            exitPoolChannel = RECREATIONAL_EXIT_CHANNEL;
            break;
        case 2:
            enterPoolChannel = KIDS_ENTER_CHANNEL;
            exitPoolChannel = KIDS_EXIT_CHANNEL;
            break;
        case 3:
            enterPoolChannel = OLYMPIC_ENTER_CHANNEL;
            exitPoolChannel = OLYMPIC_EXIT_CHANNEL;
            break;
        default:
            fprintf(stderr,"Pool like this, doesnt exist\n");
            exit(1);
    }
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
    if (lfgMsgID == 0)
    {
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
    }

    lfgMsg.mtype = enterPoolChannel;
    lfgMsg.pid = getpid();
    lfgMsg.age = clientData.age;
    if (clientData.hasKid)
    {
        lfgMsg.hasKid = true;
        lfgMsg.kidAge = childData.age;
    } else {lfgMsg.hasKid = false;}

    //printf("Klient wysyła wiadomość na kanał: %ld\n", lfgMsg.mtype);



    // Wysłanie zapytania do ratownika
    printf("Klient PID: %d wysyła wiadomość na kanał: %ld\n", getpid(),lfgMsg.mtype);
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
        //printf("Klient PID: %d wchodzi na basen %d!\n", getpid(), enterPoolChannel);
        return true;
    } else {
        //printf("Klient PID: %d nie wchodzi na basen %d.\n", getpid(), enterPoolChannel);
        return false;
    }




}

bool ExitPool() {

    lfgMsg.mtype = exitPoolChannel;
    lfgMsg.pid = getpid();
    lfgMsg.age = clientData.age;

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
        //printf("Klient PID: %d wychodzi z basenu %d!\n", getpid(), enterPoolChannel);
        clientData.inPool = false;
        return true;
    } else {
        return false;
    }
}

void *MonitorTime(void *arg) {
    time_t start_time = *(time_t *)arg;  // Czas startu procesu
    time_t current_time;
    double elapsed_time;
    struct tm *local_time;

    while (true) {
        current_time = time(NULL);  // Pobieramy aktualny czas w sekundach od epoki UNIX
        elapsed_time = difftime(current_time, start_time);
        // Sprawdzanie, czy minęło 10 sekund
        if (elapsed_time >= 10) {
            //printf("Minęło 5 sekund od startu programu!\n");
            raise(34);
            break;
        }

        // Sprawdzanie, czy jest godzina 18:00
        local_time = localtime(&current_time);  // Konwersja na czas lokalny
        if (local_time->tm_hour == 18 && local_time->tm_min == 0 && local_time->tm_sec == 0) {
            printf("Jest dokładnie godzina 18:00!\n");
        }
    }

    return NULL;
}

void *ChildThread(void *arg) {
    printf("Jestem dzieckiem!\n");

    while (!leaveFlag) {
    }
    return NULL;
}

void TimeHandler(int sig) {
    printf("Sygnal do wyjscia PID: %d!\n", getpid());

    leaveFlag = true;

    if (clientData.inPool) {
        printf("Probuje wyjsc z basenu, PID: %d!\n", getpid());
        ExitPool();
    }

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);

    //exit(0);

}

void ExitPoolHandler(int sig) {
    printf("\033[0;31;49mMam wyjsc z wody\033[0m\n");
    ExitPool();

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
}