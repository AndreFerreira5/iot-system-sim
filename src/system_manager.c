#include "system_manager.h"
#include "queue.h"
#include "log.h"
#include "ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

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


    // set internal queue size
    set_queue_size();

    // create internal queue to hold tasks (change this)
    Task *INTERNAL_QUEUE;

    while(1){
        sleep(1);
        request_log("INFO", "This is an example log!");
        //printf("ring buffer: %s\n", ring_buffer_shmem->ring_buffer.buffer);
    }

    exit(0);
}