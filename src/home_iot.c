#include "home_iot.h"
#include "log.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

pid_t logger_pid;

void home_sigint_handler(){

    request_log("INFO", "Home_IoT shutting down - SIGINT received");
    printf("Home_IoT shutting down - SIGINT received\n");

    /* LOGGER */
    request_log("INFO", "Waiting for logger process to shutdown");
    printf("WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // close logger process as last for all the logs to be logged
    if(kill(logger_pid, SIGINT) == -1){
        perror("kill sigint");
        kill(logger_pid, SIGKILL);
    }

    //wait for logger process to finish
    waitpid(logger_pid, 0, 0);

    unload_config_file();
    exit(0);
}

int main(int argc, char *argv[]){
  
    if(argc != 2){
          printf("home_iot *CONFIG_FILE*");
          return 1;
    }

    load_config_file(argv[1]);
  
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
        //wait for logger_pid to boot
        sleep(1);
        request_log("EXAMPLE", "This is an example log");
    }
    waitpid(logger_pid, 0, 0);
    return 0;
};