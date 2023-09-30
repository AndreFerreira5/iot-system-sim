#ifndef IOT_SYSTEM_SIM_SYSTEM_MANAGER_H
#define IOT_SYSTEM_SIM_SYSTEM_MANAGER_H

#include "ring_buffer.h"

typedef struct{
    ring_buffer_t ring_buffer;
}shared_ring_buffer;

/* */
void init_sys_manager();

#endif //IOT_SYSTEM_SIM_SYSTEM_MANAGER_H
