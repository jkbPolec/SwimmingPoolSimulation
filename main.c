#include "common.h"

void createCashier();
void createClients();
void createLifeguards();

int main() {

    //-------------------Tworzenie kasjera
    createCashier();

    //-------------------Tworzenie klientow
    createClients();

    //-------------------Tworzenie ratowników
    createLifeguards();

    //-------------------Czekanie az wszystkie procesy sie skoncza
    for (int i = 0; i < NUM_CLIENTS + 1; i++) {
        wait(NULL);
    }

    printf("Wszystkie procesy zakończone.\n");

    return 0;
}

void createCashier() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed for kasjer");
        exit(1);
    }

    if (pid == 0) {
        printf("Tworzę proces kasjera...\n");
        execl("./kasjer", "kasjer", (char *)NULL);
        perror("execl kasjer failed");
        exit(1);
    }
}

void createClients() {
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

void createLifeguards() {
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
