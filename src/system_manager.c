#include "system_manager.h"
#include "log.h"
#include "ring_buffer.h"
#include "bin_heap.h"
#include "config.h"
#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>

void sys_sigint_handler(){

    request_log("INFO", "System Manager shutting down - SIGINT received");
    printf("System Manager shutting down - SIGINT received\n");

    exit(0);
}

void sys_error_handler(){
    request_log("INFO", "System Manager shutting down - SIGINT received");
    printf("System Manager shutting down - Internal Error\n");

    kill(getppid(), SIGINT);
    exit(1);
}

_Noreturn void init_sys_manager(){

    request_log("INFO", "SYSTEM MANAGER BOOTING UP");

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sys_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }



    size_t num_workers = get_config_value("N_WORKERS");
    pid_t workers_pid[num_workers];
    for(size_t i=0; i<num_workers; i++) {
        if ((workers_pid[i] = fork()) == 0) {
            init_worker();
        }
    }

    // wait for all worker processes
    for(size_t i=0; i<num_workers; i++) {
        waitpid(workers_pid[i], 0, 0);
    }

    exit(0);
}