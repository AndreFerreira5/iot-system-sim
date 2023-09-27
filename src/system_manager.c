#include "system_manager.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void sigint_handler(){
    //TODO sigint all worker processes
    exit(1);
}

void init_sys_manager(){

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigterm signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    // set internal queue size
    set_queue_size();

    // create internal queue to hold tasks (change this)
    Task *INTERNAL_QUEUE;
}