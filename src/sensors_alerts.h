#ifndef IOT_SYSTEM_SIM_SENSORS_ALERTS_H
#define IOT_SYSTEM_SIM_SENSORS_ALERTS_H

#include <pthread.h>
#include <string.h>

typedef struct{
    char id[32];
    char key[32];
    long value;
}sensor;

typedef struct{
    char id[32];
    long min;
    long max;
}alert;

typedef struct{
    sensor* sensors;
    size_t* sensor_indices;
    int active_sensors;
    int max_sensors;
    alert* alerts;
    pthread_mutex_t shmem_mutex;
}sensors_alerts;


size_t hash_sensor_id(const char* id, int max_sensors){
    unsigned long hash = 5381; // magic hash number
    int c;

    while((c = *id++)){
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % max_sensors;
}


int insert_sensor_hash_table(sensors_alerts* sa, sensor new_sensor){
    // if the number of max sensors has been reached return 0
    if(sa->active_sensors == sa->max_sensors) return 0;

    // insert sensor on sensors array in the first slot available
    sa->sensors[sa->active_sensors] = new_sensor;

    // hash the sensor's ID to find the index in sensorIndices
    size_t index = hash_sensor_id(new_sensor.id, sa->max_sensors);

    // linear probing for collision resolution
    while(sa->sensor_indices[index] != -1){
        // find the next available slot in case of collision
        index = (index+1) % sa->max_sensors;
    }

    // link the hashed index to the sensor's position in the sensors array
    sa->sensor_indices[index] = sa->max_sensors;

    // increment number of active sensors
    sa->active_sensors++;
    return 1;
}


sensor* find_sensor_hash_table(sensors_alerts* sa, const char* id){
    size_t index = hash_sensor_id(id, sa->max_sensors);

    while(sa->sensor_indices[index] != -1){
        if(strcmp(sa->sensors[sa->sensor_indices[index]].id, id) == 0){
            return &sa->sensors[sa->sensor_indices[index]];
        }
        index = (index + 1) % sa->max_sensors; // linear probing
    }

    return NULL;
}


void compact_sensors_array(sensors_alerts* sa, size_t sensor_position){
    for (size_t i = sensor_position; i < sa->max_sensors - 1; i++) {
        sa->sensors[i] = sa->sensors[i + 1];
        // Update sensorIndices for the shifted sensor
        size_t shifted_sensor_hash_index = hash_sensor_id(sa->sensors[i].id, sa->max_sensors);
        sa->sensor_indices[shifted_sensor_hash_index] = i;
    }
}


void remove_sensor_hash_table(sensors_alerts* sa, const char* id){
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
