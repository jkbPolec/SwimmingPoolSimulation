#include "common.h"

void CreateCashier();
void CreateClients();
void CreateLifeguards();
void CreateManager();

void sigintHandler(int sig);
void terminateProcesses();

int pids[5];

int main() {
    signal(SIGINT, sigintHandler);


    //-------------------Tworzenie kasjera
    CreateCashier();

    //-------------------Tworzenie ratowników
    CreateLifeguards();


    //-------------------Tworzenie menedźera
    CreateManager();

    sleep(1);
    printf("\033[1;32;40m---------------Początek pracy basenu---------------\033[0m\n");
    //-------------------Tworzenie klientow
    CreateClients();


    //-------------------Czekanie az wszystkie procesy sie skoncza
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int status;
        pid_t pid = wait(&status); // Oczekuj na zakończenie dowolnego dziecka

        if (pid > 0) {
            if (WIFEXITED(status)) {
                printf("Proces o PID %d zakończył się z kodem wyjścia %d.\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Proces o PID %d zakończył się sygnałem %d.\n", pid, WTERMSIG(status));
            } else {
                printf("Proces o PID %d zakończył się w nieoczekiwany sposób.\n", pid);
            }
        } else {
            perror("wait");
        }
    }

    terminateProcesses();


    return 0;
}

void CreateCashier() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed for kasjer");
        exit(1);
    }
    pids[0] = pid;
    if (pid == 0) {
        printf("Tworzę proces kasjera...\n");
        execl("./kasjer", "kasjer", (char *)NULL);
        perror("execl kasjer failed");
        exit(1);
    }
}

void CreateClients() {
    pid_t pid;
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed for klient");
            exit(1);
        }

        if (pid == 0) {
            printf("Tworzę proces klienta %d...\n", i + 1);
            execl("./klient", "klient", (char *)NULL);
            perror("execl klient failed");
            exit(1);
        }
    }
}

void CreateLifeguards() {
    const int poolCodes[] = {RECREATIONAL_POOL_CODE, KIDS_POOL_CODE, OLYMPIC_POOL_CODE};
    const char *messages[] = {
            "Tworzę proces ratownika b.rekreacyjnego ...\n",
            "Tworzę proces ratownika b.brodzika ...\n",
            "Tworzę proces ratownika b.olimpijskiego ...\n"
    };

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed for ratownik");
            exit(1);
        }
        pids[i + 1] = pid;

        if (pid == 0) {
            printf("%s", messages[i]);

            char str_value[4];
            snprintf(str_value, sizeof(str_value), "%d", poolCodes[i]);
            execl("./ratownik", "ratownik", str_value, (char *)NULL);

            perror("execl ratownika failed");
            exit(1);
        }
    }
}

void CreateManager() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed for ratownik");
        exit(1);
    }

    if (pid == 0) {
        pids[4] = pid;
        printf("Tworzę proces menadżera\n");
        char pidCashier[10], pidLfgr1[10], pidLfgr2[10], pidLfgr3[10];
        sprintf(pidCashier, "%d", pids[0]);
        sprintf(pidLfgr1, "%d", pids[1]);
        sprintf(pidLfgr2, "%d", pids[2]);
        sprintf(pidLfgr3, "%d", pids[3]);

        execl("./manager", "manager", pidCashier, pidLfgr1, pidLfgr2, pidLfgr3, (char *)NULL);

        perror("execl ratownika failed");
        exit(1);
    }

}

// Funkcja do zakończenia procesów
void terminateProcesses() {
    for (int i = 0; i < 5; ++i) {
        kill(pids[i], SIGKILL);
    }
}


void sigintHandler(int sig) {
    printf("\nCtrl+C wciśnięte. Zakończenie procesów...\n");
    terminateProcesses();
    system("./clean.sh");
    exit(0); // Opcjonalnie zakończ program po zakończeniu procesów
}