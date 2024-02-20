#include "worker.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>

_Noreturn void init_worker(sensors_alerts* sensors_alerts_shmem, maxHeap* taskHeap){
    long pid = (long)getpid();

    request_log("INFO", "WORKER %ld BOOTING UP", pid);
#ifdef DEBUG
    fprintf(stdout, "WORKER %ld BOOTING UP\n", pid);
#endif

    while(1){

        #ifdef DEBUG
        fprintf(stdout, "[WORKER %ld] GETTING TASK IN HEAP\n", pid);
        #endif

        // get task from heap
        extract_max(taskHeap);

        #ifdef DEBUG
        fprintf(stdout, "[WORKER %ld] GOT TASK FROM HEAP\n", pid);
        #endif



        // lock shmem mutex
        pthread_mutex_unlock(&sensors_alerts_shmem->shmem_mutex);

        // unlock shmem mutex
        pthread_mutex_lock(&sensors_alerts_shmem->shmem_mutex);

    };
}