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
void *QueueRoutine(void*);
void *EnterPool(void*);
void ExitPool();
void SemaphoreAction(int action);



void* ChildThread(void*);
void* MonitorTime(void*);
void *ExitPoolThread(void* arg);
void TimeHandler(int sig);
void ExitPoolHandler(int sig);


struct ClientData clientData;
struct ChildData childData;
pthread_t moniotorThread, childThread, msgThread, testExitThread;
bool leaveFlag = false;
bool leaveReqSent = false;

int enterPoolChannel,exitPoolChannel, chosenPool;
int lfgMsgID,  msgKey, semID;
struct LifeguardMessage lfgMsg;


int main() {

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);

    SetUpClient();


    if (!clientData.isVIP) {

        pthread_create(&msgThread, NULL, QueueRoutine, "");
        pthread_join(msgThread, NULL);
    }

    SemaphoreAction(1);




    time_t start_time = time(NULL);
    if(pthread_create(&moniotorThread, NULL, MonitorTime, (void*)&start_time) != 0)
    {
        perror("pthread_create monitor");
        exit(17);
    }


//    ChoosePool();
//    pthread_create(&msgThread, NULL, EnterPool, "");
//    pthread_join(msgThread, NULL);


    while (!leaveFlag) {

        while (!clientData.inPool && !leaveFlag) {
                ChoosePool();
                pthread_create(&msgThread, NULL, EnterPool, "");
                pthread_join(msgThread, NULL);
        }
    }
//    printf("Klient %d Checkpoint\n", getpid());

    pthread_join(msgThread, NULL);
//    printf("Watek joined %d \n", getpid());
    if (clientData.inPool)
    {
        ExitPool();
    }


//    // Czekanie na zakończenie wątku monitora czasu
//    if (pthread_join(moniotorThread, NULL) != 0) {
//        perror("pthread_join monitor");
//    }
//
//    // Czekanie na zakończenie wątku dziecka, jeśli istnieje
//    if (clientData.hasKid) {
//        if (pthread_join(childThread, NULL) != 0) {
//            perror("pthread_join child");
//        }
//    }

//    printf("Klient %d konczy prace\n", getpid());
    SemaphoreAction(-1);
    exit(0);
}



void SetUpClient() {

    // Łączenie się z istniejącym semaforem
    semID = semget(SEM_KEY, 1, 0666);
    if (semID == -1) {
        perror("semget");
        exit(2);
    }


    srand(time(NULL) + getpid());
    //TODO dodac dziecko, zmienic zakres wieku
    clientData.age = rand() % 70 + 1;
    //clientData.age = 5;
    if (clientData.age < 10) {
        clientData.age = rand() % 41 + 30;
        clientData.hasKid = true;
        childData.age = rand() % 9 + 1;
        //TODO USUNAC
        childData.age = 3;
        if (pthread_create(&childThread, NULL, ChildThread, NULL) != 0) {
            perror("[dziecko] pthread_create");
            exit(3);
        }

    } else { clientData.hasKid = false;}
    ChoosePool();

    int VIP = rand() % 100 + 1;
    if (VIP == 1) {clientData.isVIP = true;}
    clientData.inPool = false;
}

void SemaphoreAction(int action) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = action;  // Operacja zwiększenia/zmniejszenia o 1
    op.sem_flg = 0;

    if (semop(semID, &op, 1) == -1) {
        perror("semop");
        exit(4);
    }

    int semValue = semctl(semID, 0, GETVAL);
    if (semValue == -1) {
        perror("semctl getval");
        exit(5);
    }
    if (semValue%10 == 0) {
        printf("[KLI %d][%d]Aktualna wartość semafora: %d\n", getpid(), action,semValue);
    }

    //printf("Klient PID %d zwiększył wartość semafora.\n", getpid());
}

void ChoosePool() {
    chosenPool = rand() % 3 + 1;
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
            exit(6);
    }
}

void *QueueRoutine(void* arg) {
    int key, msgid;
    struct message msg;

    // Generowanie klucza do kolejki
    key = ftok("queuefile", 65);
    if (key == -1) {
        perror("ftok");
        exit(7);
    }

    // Otwieranie kolejki komunikatów
    msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(8);
    }

    // Wysyłanie komunikatu z PID-em klienta do kasjera
    msg.mtype = CASHIER_CHANNEL;
    msg.pid = getpid();

    if (clientData.hasKid) {
        msg.kidAge = childData.age;
    }
    else
    {
        msg.kidAge = 99;
    }


    struct msqid_ds queueInfo;

    while (1) {
        if (msgctl(msgid, IPC_STAT, &queueInfo) == -1) {
            perror("msgctl");
            exit(18);
        }

        if (queueInfo.msg_qnum < MAX_CLIENT_MSG)
        {
            break;
        }
    }

    if (msgsnd(msgid, &msg, sizeof(msg.pid), 0) == -1) {
        perror("msgsnd");
        exit(9);
    }
//    printf("Klient (PID: %d) wysłał swój PID do kasjera.\n", msg.pid);
    msg.mtype = msg.pid;

    // Oczekiwanie na odpowiedź od kasjera
    if (msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), msg.pid, 0) == -1) {
        perror("msgrcv");
        exit(10);
    }

    if (msg.allowed == 0) {
        printf("Klient (PID: %d): Basen zamkniety, ide do domu\n", msg.pid);
        leaveFlag = true;
    }
    else{
        printf("Klient (PID: %d) otrzymał odpowiedź od kasjera.\n", msg.pid);
    }

    return;


}

void *EnterPool(void* arg)
{


        if (lfgMsgID == 0) {
            //-------------------Tworzenie kolejki komunikatow do basenu
            msgKey = ftok("queuefile", 65);
            if (msgKey == -1) {
                perror("ftok");
                exit(11);
            }

            lfgMsgID = msgget(msgKey, 0600);
            if (lfgMsgID == -1) {
                perror("msgget");
                exit(12);
            }
        }

        lfgMsg.mtype = enterPoolChannel;
        lfgMsg.pid = getpid();
        lfgMsg.age = clientData.age;
        if (clientData.hasKid) {
            lfgMsg.hasKid = true;
            lfgMsg.kidAge = childData.age;
        } else { lfgMsg.hasKid = false; }

        //printf("Klient wysyła wiadomość na kanał: %ld\n", lfgMsg.mtype);



        struct msqid_ds queueInfo;

        while (1) {
            if (msgctl(lfgMsgID, IPC_STAT, &queueInfo) == -1) {
                perror("msgctl");
                exit(18);
            }

            if (queueInfo.msg_qnum < MAX_CLIENT_MSG)
            {
                break;
            }
        }

        // Wysłanie zapytania do ratownika
//        printf("Klient PID: %d wysyła wiadomość na kanał: %ld\n", getpid(),lfgMsg.mtype);
        if (msgsnd(lfgMsgID, &lfgMsg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(13);
        }

        while (1) {
//            printf("Proba wejscia na basen %d\n", getpid());
            if (msgrcv(lfgMsgID, &lfgMsg, sizeof(struct LifeguardMessage) - sizeof(long), getpid(), 0) == -1) {
                if (errno == EINTR) {
                    printf("Przerwanie msgrcv\n");
                    continue;

                } else {
                    perror("msgrcv");
                    exit(14);
                }
            }
            break;
        }

//        printf("Odebrana wiadomosc %d\n", getpid());


        // Wyświetlenie wyniku

        if (lfgMsg.allowed) {
            //printf("Klient PID: %d wchodzi na basen %d!\n", getpid(), enterPoolChannel);
            clientData.inPool = true;
        } else {
            //printf("Klient PID: %d nie wchodzi na basen %d.\n", getpid(), enterPoolChannel);
            clientData.inPool = false;
        }

        if (leaveFlag) {
//            printf("Ostatni enter pool %d\n", getpid());
        }

    return;
}

void ExitPool() {



    pthread_create(&testExitThread, NULL, ExitPoolThread, "");
    pthread_join(testExitThread, NULL);
//    printf("Klient %d Checkpoint 2\n", getpid());

}

void *ExitPoolThread(void* arg) {
    leaveReqSent = true;


    lfgMsg.mtype = exitPoolChannel;
    lfgMsg.pid = getpid();
    lfgMsg.age = clientData.age;

//    printf("Klient wysyła wiadomość na kanał: %ld\n", lfgMsg.mtype);


    struct msqid_ds queueInfo;

    while (1) {
        if (msgctl(lfgMsgID, IPC_STAT, &queueInfo) == -1) {
            perror("msgctl");
            exit(18);
        }

        if (queueInfo.msg_qnum < MAX_CLIENT_MSG)
        {
            break;
        }
    }

    // Wysłanie zapytania do ratownika
    if (msgsnd(lfgMsgID, &lfgMsg, sizeof(struct LifeguardMessage) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(15);
    }

    while (1) {
//        printf("Proba wyjscia z basenu %d\n", getpid());
        if (msgrcv(lfgMsgID, &lfgMsg, sizeof(struct LifeguardMessage) - sizeof(long), getpid(), 0) == -1) {
            if (errno == EINTR) {
                printf("Przerwanie msgrcv\n");
                continue;

            } else {
                perror("msgrcv");
                exit(16);
            }
        }
        break;
    }


    // Wyświetlenie wyniku
    if (lfgMsg.allowed) {
        //printf("Klient PID: %d wychodzi z basenu %d!\n", getpid(), enterPoolChannel);
        clientData.inPool = false;
    }
    leaveReqSent = false;

//    printf("Ostatni exit pool %d\n", getpid());
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
        if (elapsed_time >= 5 && leaveFlag == false) {
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

//    if (clientData.inPool && leaveReqSent == false) {
//        printf("Probuje wyjsc z basenu, PID: %d!\n", getpid());
//        pthread_create(&testExitThread, NULL, ExitPoolThread, "");
//        pthread_join(testExitThread, NULL);
//    }

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
    //exit(0);
}

void ExitPoolHandler(int sig) {
    printf("\033[0;31;49mMam wyjsc z wody\033[0m\n");
    if (leaveReqSent == false)
    {
        pthread_create(&testExitThread, NULL, ExitPoolThread, "");
        pthread_join(testExitThread, NULL);
    }

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
}