#include "worker.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>


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
        if(latest_value > sensor_info->min) sensor_info->min = latest_value;
    }
        // if the hash table has reached the max sensor size, remove the sensor that hasnt responded in the longest time (possibly the DEAD signal has been dropped and didnt reach the workers)
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


        fprintf(stdout, "[WORKER %ld] UNLOCKING MUTEX\n", pid);
        // lock shmem mutex
        pthread_mutex_unlock(&sensors_alerts_shmem->shmem_mutex);
        fprintf(stdout, "[WORKER %ld] UNLOCKED MUTEX\n", pid);

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

        fprintf(stdout, "[WORKER %ld] LOCKING MUTEX\n", pid);
        // unlock shmem mutex
        pthread_mutex_lock(&sensors_alerts_shmem->shmem_mutex);
        fprintf(stdout, "[WORKER %ld] LOCKED MUTEX\n", pid);

    }
}