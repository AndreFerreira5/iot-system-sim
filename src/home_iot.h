#ifndef IOT_SYSTEM_SIM_HOME_IOT_H
#define IOT_SYSTEM_SIM_HOME_IOT_H

#include "ring_buffer.h"
#include <sys/types.h>

typedef struct{
    ring_buffer_t ring_buffer;
}shared_ring_buffer;
extern shared_ring_buffer *ring_buffer_shmem;

#endif //IOT_SYSTEM_SIM_HOME_IOT_H
