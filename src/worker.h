#ifndef IOT_SYSTEM_SIM_WORKER_H
#define IOT_SYSTEM_SIM_WORKER_H

#include "sensors_alerts.h"
#include "max_heap.h"

_Noreturn void init_worker(sensors_alerts* sensors_alerts_shmem, maxHeap* taskHeap);

#endif //IOT_SYSTEM_SIM_WORKER_H
