#ifndef IOT_SYSTEM_SIM_SENSORS_ALERTS_H
#define IOT_SYSTEM_SIM_SENSORS_ALERTS_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "max_heap.h"

typedef struct{
    char id[33];
    char key[33];
    long min, max, latest;
}sensor_info;

typedef struct{
    char id[32];
    long min;
    long max;
}alert;

typedef struct{
    sensor_info* sensors;
    size_t* sensor_indices;
    long active_sensors;
    long max_sensors;
    alert* alerts;
    pthread_mutex_t shmem_mutex;
}sensors_alerts;


static inline size_t hash_sensor_id(const char* id, long max_sensors){
    unsigned long hash = 5381; // magic hash number
    int c;

    while((c = *id++)){
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % max_sensors;
}


static inline int insert_sensor_hash_table(sensors_alerts* sa, sensor new_sensor){
    // if the number of max sensors has been reached return 0
    if(sa->active_sensors == sa->max_sensors) return 0;

    // init a sensor_info struct with the provided sensor info
    // and insert it on sensors array in the first slot available
    sensor_info sensor_info;
    strcpy(sensor_info.id, new_sensor.id);
    strcpy(sensor_info.key, new_sensor.key);
    sensor_info.min = sensor_info.max = sensor_info.latest = new_sensor.value;
    sa->sensors[sa->active_sensors] = sensor_info;

    // hash the sensor's ID to find the index in sensorIndices
    size_t index = hash_sensor_id(new_sensor.id, sa->max_sensors);

    // linear probing for collision resolution
    while(sa->sensor_indices[index] != -1){
        // find the next available slot in case of collision
        index = (index+1) % sa->max_sensors;
    }

    // link the hashed index to the sensor's position in the sensors array
    sa->sensor_indices[index] = sa->active_sensors;

    // increment number of active sensors
    sa->active_sensors++;
    return 1;
}


static inline sensor_info* find_sensor_hash_table(sensors_alerts* sa, const char* id){
    size_t index = hash_sensor_id(id, sa->max_sensors);

    while(sa->sensor_indices[index] != -1){
        fprintf(stdout, "%s - %s\n", sa->sensors[sa->sensor_indices[index]].id, id);
        if(strcmp(sa->sensors[sa->sensor_indices[index]].id, id) == 0){
            return &sa->sensors[sa->sensor_indices[index]];
        }
        index = (index + 1) % sa->max_sensors; // linear probing
    }

    return NULL;
}


static inline void compact_sensors_array(sensors_alerts* sa, size_t sensor_position){
    for (size_t i = sensor_position; i < sa->max_sensors - 1; i++) {
        sa->sensors[i] = sa->sensors[i + 1];
        // Update sensorIndices for the shifted sensor
        size_t shifted_sensor_hash_index = hash_sensor_id(sa->sensors[i].id, sa->max_sensors);
        sa->sensor_indices[shifted_sensor_hash_index] = i;
    }
}


static inline void remove_sensor_hash_table(sensors_alerts* sa, const char* id){
    size_t index = hash_sensor_id(id, sa->max_sensors);
    size_t sensor_position = -1;

    while (sa->sensor_indices[index] != -1) {
        if (strcmp(sa->sensors[sa->sensor_indices[index]].id, id) == 0) {
            sensor_position = sa->sensor_indices[index];
            break;
        }
        // move to the next index in case of collision
        index = (index + 1) % sa->max_sensors;
    }

    // if sensor not found, return
    if (sensor_position == -1) return;

    // decrement number of active sensors
    sa->active_sensors--;
    // mark hash table entry as empty
    sa->sensor_indices[index] = -1;

    compact_sensors_array(sa, sensor_position);
}


#endif //IOT_SYSTEM_SIM_SENSORS_ALERTS_H
