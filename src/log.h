#ifndef IOT_SYSTEM_SIM_LOG_H
#define IOT_SYSTEM_SIM_LOG_H

#include "system_manager.h"

#define LOG_PIPE "/tmp/LOG_PIPE"

_Noreturn void init_logger(shared_ring_buffer *ring_buffer_shmem);
void request_log(char* type, char* message);
void create_fifo();

#endif //IOT_SYSTEM_SIM_LOG_H