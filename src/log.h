#ifndef IOT_SYSTEM_SIM_LOG_H
#define IOT_SYSTEM_SIM_LOG_H

#include <stddef.h>
#include "system_manager.h"

typedef struct{
    char **types;
    char **messages;
    size_t count;
    size_t capacity;
} temp_log_buffer_t;
extern temp_log_buffer_t temp_log_buffer;

_Noreturn void init_logger();

/* Sends a log request to the logger process */
void request_log(char* type, const char* format, ...);

/* Sends a log request to the logger process but doesn't blindly send the request to the ring buffer
 * and checks if it is initialized. If not, it sends the request to a temporary buffer that gets
 * flushed to the ring buffer when it gets initialized */
void request_log_safe(char* type, const char* format, ...);

#endif //IOT_SYSTEM_SIM_LOG_H