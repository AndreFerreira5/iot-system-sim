#ifndef IOT_SYSTEM_SIM_SENSORS_ALERTS_H
#define IOT_SYSTEM_SIM_SENSORS_ALERTS_H

#include <pthread.h>

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

#endif //IOT_SYSTEM_SIM_SENSORS_ALERTS_H
