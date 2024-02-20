#ifndef IOT_SYSTEM_SIM_HOME_IOT_H
#define IOT_SYSTEM_SIM_HOME_IOT_H

#include "ring_buffer.h"
#include <sys/types.h>

typedef struct{
    ring_buffer_t ring_buffer;
}shared_ring_buffer;
extern shared_ring_buffer *ring_buffer_shmem;

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
    alert* alerts;
    pthread_mutex_t shmem_mutex;
}sensors_alerts;

#endif //IOT_SYSTEM_SIM_HOME_IOT_H
