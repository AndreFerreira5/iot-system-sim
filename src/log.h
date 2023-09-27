#ifndef IOT_SYSTEM_SIM_LOG_H
#define IOT_SYSTEM_SIM_LOG_H

#define LOG_PIPE "/tmp/LOG_PIPE"

_Noreturn void init_logger();
void request_log(char* type, char* message);


#endif //IOT_SYSTEM_SIM_LOG_H