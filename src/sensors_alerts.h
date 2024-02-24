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


/* Hashes the provided sensor ID and returns the division remainder between it and the max_sensors value,
 * effectively returning a number between the bounds of the sensors struct array */
static inline size_t hash_sensor_id(const char* id, long max_sensors){
    unsigned long hash = 5381; // magic hash number
    int c;

    while((c = *id++)){
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
#ifdef DEBUG
    fprintf(stdout, "HASHED SENSOR WITH ID %s: %zd\n", id, index);
#endif
    return hash % max_sensors;
}


/* Iterates through the provided sensors struct array and returns the index of the first available spot (uninitialized sensor) */
static inline size_t find_first_available_sensor_spot(sensors_alerts* sa){
    size_t i = 0;
    // iterate through the sensors array
    for(; i<sa->max_sensors; i++){
        // if the sensor is not initialized
        if(sa->sensors[i].id[0] == '\0'){
            // return its position
            return i;
        }
    }

    // if there isn't an uninitialized sensor, return -1
    return -1;
}


/* Inserts sensor on hash table, creating a new sensor_info struct and adding it to the first spot available on the sensors struct array,
 * with the hash entry value being the index of the new sensor on the struct array
 * Returns 0 in case of the number of sensors reaching the max
 * Returns 1 in case fo success */
static inline int insert_sensor_hash_table(sensors_alerts* sa, sensor new_sensor){
    // if the number of max sensors has been reached, return 0
    if(sa->active_sensors == sa->max_sensors) return 0;

    // get first available spot on sensor array
    size_t sensors_array_index = find_first_available_sensor_spot(sa);
    if(sensors_array_index == -1) return 0;

    // init a sensor_info struct with the provided sensor info
    sensor_info sensor_info;
    strcpy(sensor_info.id, new_sensor.id);
    strcpy(sensor_info.key, new_sensor.key);
    sensor_info.min = sensor_info.max = sensor_info.latest = new_sensor.value;

    // assign sensor in available sensors array index
    sa->sensors[sensors_array_index] = sensor_info;

    // hash the sensor's ID to find the index in sensor_indices
    size_t index = hash_sensor_id(new_sensor.id, sa->max_sensors);

    // linear probing for collision resolution
    while(sa->sensor_indices[index] != -1){
        // find the next available slot in case of collision
        index = (index+1) % sa->max_sensors;
    }
#ifdef DEBUG
    fprintf(stdout, "INSERTED SENSOR ON INDEX %zd\n", index);
#endif

    // link the hashed index to the sensor's position in the sensors array
    sa->sensor_indices[index] = sensors_array_index;

    // increment number of active sensors
    sa->active_sensors++;

    // flag successful insertion
    return 1;
}


/* Finds sensor with provided id in hash table, by hashing it and searching, comparing the sensors id
 * Returns NULL in case the sensor was not found
 * Returns the sensor_info struct if it was found */
static inline sensor_info* find_sensor_hash_table(sensors_alerts* sa, const char* id){
    size_t index = hash_sensor_id(id, sa->max_sensors);
    size_t start_index = index;

    do {
        if (strcmp(sa->sensors[sa->sensor_indices[index]].id, id) == 0) {
#ifdef DEBUG
            fprintf(stdout, "SENSOR FOUND ON INDEX %zd\n", index);
#endif
            return &sa->sensors[sa->sensor_indices[index]];
        }

        // move to the next index, wrapping around at the end of the array.
        index = (index + 1) % sa->max_sensors;

    } while (index != start_index);

#ifdef DEBUG
    fprintf(stdout, "SENSOR NOT FOUND ON HASH TABLE\n");
#endif
    return NULL;
}


/* Removes a sensor, with a provided ID, from the hash table (by setting the hash entry value to -1)
 * and from the sensors struct array (by setting the first id index with \0) */
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

    sa->sensors[sa->sensor_indices[index]].id[0] = '\0';

    // decrement number of active sensors
    sa->active_sensors--;
    // mark hash table entry as empty
    sa->sensor_indices[index] = -1;

#ifdef DEBUG
    fprintf(stdout, "SENSOR WITH ID %s REMOVED\n", id);
#endif
}


#endif //IOT_SYSTEM_SIM_SENSORS_ALERTS_H
