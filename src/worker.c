#include "worker.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

long pid;


void worker_sigint_handler(){
    request_log("INFO", "[WORKER %ld] RECEIVED SIGINT - Propagating signal upwards", pid);
    fprintf(stdout, "[WORKER %ld] RECEIVED SIGINT - Propagating signal upwards\n", pid);
    // progagate sigint upwards
    kill(SIGINT, getppid());
    exit(0);
}


void setup_worker_sigint_handler(){
    // create sigaction struct
        struct sigaction sa;
    sa.sa_handler = worker_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        request_log("ERROR", "Couldn't register SIGINT signal handler");
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }
}


void process_sensor_data(sensors_alerts* sa, sensor sensor_task){
    // if sensor sends a DEAD signal, remove it from the hash table
    if(strcmp(sensor_task.key, "DEAD")==0){
        remove_sensor_hash_table(sa, sensor_task.id);
    } else {
        fprintf(stdout, "FINDING SENSOR HASH TABLE\n");
        // get sensor from hash table
        sensor_info* sensor_info = find_sensor_hash_table(sa, sensor_task.id);

        // if the sensor is not on the hash table, insert it
        if(sensor_info == NULL){
            fprintf(stdout, "INSERTING SENSOR IN HASH TABLE\n");
            insert_sensor_hash_table(sa, sensor_task);
            return;
        }

        // update sensor_info with the latest sensor value
        long latest_value = sensor_task.value;
        sensor_info->latest = latest_value;
        if(latest_value > sensor_info->max) sensor_info->max = latest_value;
        if(latest_value < sensor_info->min) sensor_info->min = latest_value;
    }
}


void process_user_console_data(node* task){

}


_Noreturn void init_worker(sensors_alerts* sensors_alerts_shmem, maxHeap* taskHeap){
    setup_worker_sigint_handler();

    pid = (long)getpid();

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

        if(task.type == INVALID) continue;

        // lock shmem mutex
        pthread_mutex_lock(&sensors_alerts_shmem->shmem_mutex);

        switch(task.type){
            case SENSOR_DATA:
                process_sensor_data(sensors_alerts_shmem, task.sensor);
                break;
            case USER_CONSOLE_DATA:
                // process_user_console_data(&task);
                break;
            default:
                break;
        }

        // unlock shmem mutex
        pthread_mutex_unlock(&sensors_alerts_shmem->shmem_mutex);
    }
}