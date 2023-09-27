#include "home_iot.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

pid_t logger_pid;

void home_sigint_handler(){

    // close logger process as last for all the logs to be logged
    if(kill(logger_pid, SIGINT) == -1){
        perror("kill sigint");
        kill(logger_pid, SIGKILL);
    }
    printf("WAITING FOR LOGGER PID TO SHUTDOWN\n");
    waitpid(logger_pid, 0, 0);
}

int main(){
    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = home_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigterm signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    if(sigaction(SIGTERM, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    if ((logger_pid = fork()) == 0){
        init_logger();
    }
    else{

    }
    waitpid(logger_pid, 0, 0);
    return 0;
};