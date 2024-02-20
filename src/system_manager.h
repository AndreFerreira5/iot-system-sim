#ifndef IOT_SYSTEM_SIM_SYSTEM_MANAGER_H
#define IOT_SYSTEM_SIM_SYSTEM_MANAGER_H
#include "max_heap.h"

typedef struct{
    char* sensorFIFO;
    maxHeap* taskHeap;
} SensorReaderThreadArgs;

/* System Manager process initialization function */
_Noreturn void init_sys_manager(char* sensorFIFO, sensors_alerts* sensors_alerts_shmem);

#endif //IOT_SYSTEM_SIM_SYSTEM_MANAGER_H
