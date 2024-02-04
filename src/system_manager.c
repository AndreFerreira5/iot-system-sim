#include "system_manager.h"
#include "log.h"
#include "max_heap.h"
#include "config.h"
#include "worker.h"
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

    // signal parent process (home_iot) to stop
    kill(getppid(), SIGINT);
    exit(1);
}

void setup_sigint_handler(){
    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sys_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        request_log("ERROR", "Couldn't register SIGINT signal handler");
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        sys_error_handler();
    }
}

_Noreturn void init_sys_manager(){
    request_log("INFO", "SYSTEM MANAGER BOOTING UP");


    /* Task Heap creation */
    int heap_capacity = get_config_value("HEAP_CAPACITY");
    if(heap_capacity <= 0){ // if the config value is invalid, exit
        request_log("ERROR", "HEAP_CAPACITY config value invalid (must be bigger than 0)");
        sys_error_handler();
    }
    maxHeap* taskHeap = create_heap(heap_capacity);


    /* Worker processes spawning */
    size_t num_workers = get_config_value("NUM_WORKERS");
    if(num_workers <= 0){
        request_log("ERROR", "NUM_WORKERS config value invalid (must be bigger than 0)");
        sys_error_handler();
    }
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