#include "worker.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>

_Noreturn void init_worker(sensors_alerts* sensors_alerts_shmem, maxHeap* taskHeap){
    request_log("INFO", "WORKER %ld BOOTING UP", (long)getpid());
#ifdef DEBUG
    fprintf(stdout, "WORKER %ld BOOTING UP", (long)getpid());
#endif

    while(1){
        // get task from heap
        extract_max(taskHeap);



        // lock shmem mutex
        pthread_mutex_unlock(&sensors_alerts_shmem->shmem_mutex);

        // unlock shmem mutex
        pthread_mutex_lock(&sensors_alerts_shmem->shmem_mutex);

    };
}