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
void *QueueRoutine();
void *EnterPool();
void ExitPool();
void SemaphoreAction(int action);



void* ChildThread();
void* MonitorTime(void*);
void *ExitPoolThread();
void TimeHandler();
void ExitPoolHandler();
void CleanupResources();
void SigintHandler();

struct ClientData clientData;
struct ChildData childData;
pthread_t moniotorThread, childThread, msgThread, testExitThread;
bool leaveFlag = false;
bool leaveReqSent = false;

int enterPoolChannel,exitPoolChannel, chosenPool;
int lfgMsgID,  msgKey, semID;
struct LifeguardMessage lfgMsg;


int main() {

    //Ustawiamy obsługe sygnałów
    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
    signal(SIGINT, SigintHandler);

    //Podstawowe parametry klienta
    SetUpClient();

    //Jeśli klient nie jest vipem, ustawia sie w kolejce
    if (!clientData.isVIP) {

        pthread_create(&msgThread, NULL, QueueRoutine, "");
        pthread_join(msgThread, NULL);
    }

    //Jeśli klient wszedł na basen, zwiekszamy licznik
    SemaphoreAction(1);


    //Start watku monitorujacego, ile czasu zostalo klientowi do wyjscia
    time_t start_time = time(NULL);
    if(pthread_create(&moniotorThread, NULL, MonitorTime, (void*)&start_time) != 0)
    {
        perror("pthread_create monitor");
        exit(17);
    }

    //Dopóki klient moze byc na basenie i nie jest w wodzie, probuje wejsc do ktoregos basenu
    while (!leaveFlag) {

        while (!clientData.inPool && !leaveFlag) {
                ChoosePool();
                pthread_create(&msgThread, NULL, EnterPool, "");
                pthread_join(msgThread, NULL);
                sleep(5);
        }
    }


    pthread_join(msgThread, NULL);

    //Jesli klient w basenie, wyjdz z niego
    if (clientData.inPool)
    {
        ExitPool();
    }


    //Klient zmniejsza licznik, podczas opuszczania basenu
    SemaphoreAction(-1);

    exit(0);
}

/**
 * @brief Inicjalizuje dane klienta i dziecka.
 *
 * @note Używa globalnych zmiennych `clientData` i `childData`.
 * @exit 2 - Błąd przy uzyskaniu semafora.
 * @exit 3 - Błąd przy tworzeniu wątku dla dziecka.
 */
void SetUpClient() {

    // Łączenie się z istniejącym semaforem
    semID = semget(SEM_KEY, 1, 0666);
    if (semID == -1) {
        perror("semget");
        exit(2);
    }


    srand(time(NULL) + getpid());
    clientData.age = rand() % 70 + 1;
    if (clientData.age < 10) {
        clientData.age = rand() % 41 + 30;
        clientData.hasKid = true;
        childData.age = rand() % 9 + 1;
        if (pthread_create(&childThread, NULL, ChildThread, NULL) != 0) {
            perror("[dziecko] pthread_create");
            exit(3);
        }

    } else { clientData.hasKid = false;}
    //ChoosePool();

    int VIP = rand() % 100 + 1;
    if (VIP == 1) {clientData.isVIP = true;}
    clientData.inPool = false;
}


/**
 * @brief Wykonuje operację na semaforze i wyświetla wartość, jeśli jest wielokrotnością 10.
 *
 * @param action Operacja semafora (1 - zwiększenie, -1 - zmniejszenie).
 * @note Używa globalnego `semID`. Upewnij się, że jest zainicjalizowany.
 * @exit 4 - Błąd `semop`, 5 - Błąd `semctl`.
 */
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
        perror("semctl");
        exit(5);
    }
    if (semValue%10 == 0) {
        printf("[KLI %d][%d]Aktualna wartość semafora: %d\n", getpid(), action,semValue);
    }

//    printf("Klient PID %d zwiększył wartość semafora.\n", getpid());
}


/**
 * @brief Losuje basen (1-3) i ustawia kanały wejścia i wyjścia.
 *
 * @note Używa globalnych zmiennych `chosenPool`, `enterPoolChannel`, `exitPoolChannel`.
 * @exit 6 - Wylosowany basen nie istnieje.
 */
void ChoosePool() {
    chosenPool = rand() % 3 + 1;
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


/**
 * @brief Obsługuje ustawianie klienta w kolejce do kasjera.
 *
 */
void *QueueRoutine() {
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
    printf("[KLI %d] Ustawiam sie w kolejce\n", getpid());
    msg.mtype = msg.pid;

    // Oczekiwanie na odpowiedź od kasjera
    if (msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), msg.pid, 0) == -1) {
        perror("msgrcv");
        exit(10);
    }

    if (msg.allowed == 0) {
        printf("Klient (PID: %d): Basen \033[1;39;41mzamkniety\033[0m, ide do domu\n", msg.pid);
        leaveFlag = true;
        exit(0);
    }
    else{
        printf("[KLI %d] \033[1;39;42mwchodze\033[0m na basen\n", msg.pid);
    }

}

/**
 * @brief Wątek obsługujący wysyłanie żądania wejścia do basenu.
 *
 */
void *EnterPool()
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
        printf("[KLI %d] próbuje wejść do basenu numer %ld\n", getpid(),lfgMsg.mtype);
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

    return NULL;
}

/**
 * @brief Obsługuje proces wychodzenia klienta z basenu.
 *
 * @note Używa globalnych zmiennych `testExitThread` i `clientData`.
 */
void ExitPool() {

    pthread_create(&testExitThread, NULL, ExitPoolThread, "");
    pthread_join(testExitThread, NULL);

}

/**
 * @brief Wątek obsługujący wysyłanie żądania wyjścia z basenu.
 *
 */
void *ExitPoolThread() {
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
    return NULL;
}

/**
 * @brief Monitoruje czas pozostały klientowi na basenie.
 *
 * @param arg Wskaźnik na czas startu procesu.
 * @note Używa globalnej zmiennej `leaveFlag`.
 */
void *MonitorTime(void *arg) {
    time_t start_time = *(time_t *)arg;  // Czas startu procesu
    time_t current_time;
    double elapsed_time;

    while (true) {
        current_time = time(NULL);
        elapsed_time = difftime(current_time, start_time);
        if (elapsed_time >= 1 && leaveFlag == false) {
            raise(34);
            break;
        }
    }

    return NULL;
}

/**
 * @brief Wątek obsługujący zachowanie dziecka na basenie.
 *
 * @note Wątek działa dopóki flaga `leaveFlag` nie zostanie ustawiona na true.
 */
void *ChildThread() {
    printf("Jestem dzieckiem!\n");

    while (!leaveFlag) {
    }
    return NULL;
}

/**
 * @brief Obsługuje sygnał końca czasu dla klienta.
 *
 * @param sig Numer sygnału.
 * @note Ustawia flagę `leaveFlag` na true.
 */
void TimeHandler() {
    //printf("Sygnal do wyjscia PID: %d!\n", getpid());

    leaveFlag = true;


    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
}

/**
 * @brief Obsługuje sygnał wyjścia klienta z wody.
 *
 * @param sig Numer sygnału.
 * @note Tworzy wątek do obsługi wyjścia, jeśli flaga `leaveReqSent` jest false.
 */
void ExitPoolHandler() {
    printf("\033[0;31;49mMam wyjsc z wody\033[0m\n");

    if (leaveReqSent == false)
    {
        pthread_create(&testExitThread, NULL, ExitPoolThread, "");
        pthread_join(testExitThread, NULL);
    }

    signal(34, TimeHandler);
    signal(EXIT_POOL_SIGNAL, ExitPoolHandler);
}

/**
 * @brief Czyści zasoby używane przez program.
 *
 * @note Usuwa kolejki komunikatów i semafory.
 */
void CleanupResources() {
    printf("\033[1;33;40mUsuwanie zasobów...\033[0m\n");

    // Usuwanie kolejki komunikatów do ratowników
    if (lfgMsgID != 0 && msgctl(lfgMsgID, IPC_RMID, NULL) == -1) {
        perror("msgctl (lfgMsgID)");
    } else {
        printf("Kolejka komunikatów do ratowników usunięta.\n");
    }

    // Usuwanie semafora
    if (semID != 0 && semctl(semID, 0, IPC_RMID) == -1) {
        perror("semctl remove");
    } else {
        printf("Semafor usunięty.\n");
    }

    exit(0);
}

/**
 * @brief Obsługuje sygnał SIGINT (Ctrl+C) i zamyka zasoby.
 *
 * @param sig Numer sygnału.
 * @note Wywołuje funkcję `CleanupResources()`.
 */
void SigintHandler() {
    printf("\033[1;31;40mOtrzymano SIGINT (Ctrl+C). Zamykanie zasobów...\033[0m\n");
    CleanupResources();
}

