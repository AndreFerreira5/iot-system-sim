#include "worker.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>


void process_sensor_data(node* task){
    if(strcmp(task->sensorData->key, "DEAD")==0){
        // remove sensor from hash table
    }
    // if sensor id doesnt exist on hash table, add it
        // if the hash table has reached the max sensor size, remove the sensor that hasnt responded in the longest time (possibly the DEAD signal has been dropped and didnt reach the workers)
    // if it exists, get the array position of it and update its values
}


void process_user_console_data(node* task){

}


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
        node task = extract_max(taskHeap);

        #ifdef DEBUG
        fprintf(stdout, "[WORKER %ld] GOT TASK FROM HEAP\n", pid);
        #endif

        switch(task.type){
            case SENSOR_DATA:
                process_sensor_data(&task);
                break;
            case USER_CONSOLE_DATA:
                // process_user_console_data(&task);
                break;
            default:
                break;
        }


        // lock shmem mutex
        pthread_mutex_unlock(&sensors_alerts_shmem->shmem_mutex);

        // unlock shmem mutex
        pthread_mutex_lock(&sensors_alerts_shmem->shmem_mutex);

    };
}