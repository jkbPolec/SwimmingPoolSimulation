#include "common.h"

pid_t cashier, lifeguardRec, lifeguardKid, lifeguardOly;


int main(int argc, char* argv[]) {


//    if (argc != 5){
//        fprintf(stderr,"Niepoprawna liczba argumentow!\n");
//        exit(1);
//    }

//    for (int i = 0; i < argc; i++) {
//        printf("Argument %d: %s\n", i, argv[i]);
//    }

    cashier = (pid_t)atoi(argv[1]);
    lifeguardRec = (pid_t)atoi(argv[2]);
    lifeguardKid = (pid_t)atoi(argv[3]);
    lifeguardOly = (pid_t)atoi(argv[4]);

    int randomPid;
    while (true) {
        usleep(1250000);
        randomPid = (pid_t)atoi(argv[rand() % 3 + 2]);
        //randomPid = 2;
        printf("\033[1;31;40mWyslanie sygnalu zamkniecia\033[0m\n");

        kill(lifeguardRec,34);
        usleep(1250000);
        printf("\033[1;31;40mWyslanie sygnalu otwarcia\033[0m\n");
        kill(lifeguardRec,35);
    }

    sleep(5);
//    kill(cashier, 34);
//    kill(lifeguardRec, 34);
//    kill(lifeguardKid, 34);
//    kill(lifeguardOly, 34);

    sleep(10);


//    kill(lifeguardOly, 35);

//    kill(lifeguardOly, 34);
    return 0;
}