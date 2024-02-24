#include "home_iot.h"
#include "sensors_alerts.h"
#include "log.h"
#include "config.h"
#include "system_manager.h"
#include "worker.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <sys/mman.h>

pid_t sys_manager_pid, logger_pid;
shared_ring_buffer *ring_buffer_shmem;
size_t rbuffer_shmem_size;
void* sensors_alerts_mapped_mem;
size_t total_sensors_alerts_size;
char* sensorFIFO;
int sensorFIFODesc;

void home_sigint_handler(){

    request_log("INFO", "[HOME IOT] RECEIVED SIGINT - Shutting down");
    fprintf(stdout, "[HOME IOT] RECEIVED SIGINT - Shutting down\n");

    /* SYS MANAGER */
    request_log("INFO", "[HOME IOT] Waiting for system manager process to shutdown");
    fprintf(stdout, "[HOME IOT] WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // send SIGINT to the system manager process
    if(kill(sys_manager_pid, SIGINT) == -1){
        perror("kill sigint");
    }

    //wait for the system manager process to finish
    waitpid(sys_manager_pid, 0, 0);


    request_log("INFO", "[HOME IOT] Waiting for logger process to shutdown");
    printf("[HOME IOT] WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // close the logger process as last for all the logs to be logged
    if(kill(logger_pid, SIGINT) == -1){
        perror("kill sigint");
    }

    //wait for logger process to finish
    waitpid(logger_pid, 0, 0);

    // destroy ring buffer semaphores
    sem_destroy(&ring_buffer_shmem->ring_buffer.ring_buffer_sem);
    sem_destroy(&ring_buffer_shmem->ring_buffer.requests_count);

    // unmap shared memory
    munmap(ring_buffer_shmem, rbuffer_shmem_size);
    munmap(sensors_alerts_mapped_mem, total_sensors_alerts_size);

    close(sensorFIFODesc);
    unlink(sensorFIFO);

    unload_config_file();
    exit(0);
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Arguments missing!\nUsage: home_iot *config_file*\n");
        exit(-1);
    }

#ifdef DEBUG
    printf("IoT SIMULATOR STARTED IN DEBUG MODE\n");
#endif

    // create shared memory for ring buffer
    rbuffer_shmem_size = sizeof(ring_buffer_t);
    ring_buffer_shmem = mmap(NULL, rbuffer_shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(ring_buffer_shmem == MAP_FAILED){
        printf("SHARED MEMORY INITIALIZATION FAILED");
        //homeiot_error_handler();
        exit(1);
    }

    // create logger process
    if ((logger_pid = fork()) == 0){
        init_logger();
    }

    // create ring buffer
    ring_buffer_shmem->ring_buffer = create_ring_buffer();

    request_log_safe("INFO", "[HOME IOT] BOOTING UP");

    int config_file_load_result = load_config_file(argv[1]);
    if(config_file_load_result != SUCCESS_CONFIG_FILE_LOAD){
        switch (config_file_load_result) {
            case ERROR_OPEN_CONFIG_FILE:
                request_log_safe("ERROR", "Unable to open config file");
                fprintf(stderr, "UNABLE TO OPEN THE CONFIG FILE\n");
                break;
            case ERROR_ALLOCATE_BUFFER:
                request_log_safe("ERROR", "Couldn't allocate memory for buffer");
                fprintf(stderr, "ERROR ALLOCATING MEMORY");
                break;
            case ERROR_READ_CONFIG_FILE:
                request_log_safe("ERROR", "Couldn't read config file");
                fprintf(stderr, "ERROR READING FILE");
                break;
            case ERROR_PARSE_CONFIG_FILE:
                request_log_safe("ERROR", "Couldn't parse config file");
                fprintf(stderr, "ERROR PARSING CONFIGURATION FILE\n");
                break;
            case ERROR_ALLOCATE_CONFIG_PARAMS:
                request_log_safe("ERROR", "Couldn't allocate memory for config parameters");
                fprintf(stderr, "ERROR ALLOCATING MEMORY FOR CONFIG PARAMS\n");
                break;
            default:
                request_log_safe("ERROR", "Unknown configuration error occurred");
                fprintf(stderr, "UNKNOWN CONFIGURATION ERROR OCCURED\n");
                break;
        }

        // destroy ring buffer semaphores
        sem_destroy(&ring_buffer_shmem->ring_buffer.ring_buffer_sem);
        sem_destroy(&ring_buffer_shmem->ring_buffer.requests_count);

        // unmap shared memory
        munmap(ring_buffer_shmem, rbuffer_shmem_size);


        if(kill(logger_pid, SIGINT) == -1){
            perror("kill sigint");
        }
        //wait for logger process to finish
        waitpid(logger_pid, 0, 0);

        exit(1);
    }
    request_log_safe("INFO", "CONFIG FILE LOADED INTO MEMORY");

#ifdef DEBUG
    fprintf(stdout, "CONFIG FILE LOADED INTO MEMORY\n");
#endif

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = home_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }
    // register the sigterm signal handler
    if(sigaction(SIGTERM, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    // get maximum number of sensors and alerts
    long max_sensors = 0,
         max_alerts = 0;

    int get_config_result;
    if((get_config_result = get_config_value("MAX_SENSORS", &max_sensors, INT)) != 1){
        if(get_config_result == 0) request_log_safe("ERROR", "MAX_SENSORS config value type mismatch (INT expected)");
        else if(get_config_result == -1) request_log_safe("ERROR", "MAX_SENSORS config key not found");
        //home_iot_error_handler(); //TODO Implement this
    }

    if((get_config_result = get_config_value("MAX_ALERTS", &max_alerts, INT)) != 1){
        if(get_config_result == 0) request_log_safe("ERROR", "MAX_ALERTS config value type mismatch (INT expected)");
        else if(get_config_result == -1) request_log_safe("ERROR", "MAX_ALERTS config key not found");
        //home_iot_error_handler(); //TODO Implement this
    }

    // calculate max size of shmem using number of max sensors & alerts
    size_t size_sensors = max_sensors * sizeof(sensor_info);
    size_t size_sensor_indices = max_sensors * sizeof(size_t);
    size_t size_active_sensors = sizeof(int);
    size_t size_max_sensors = sizeof(int);
    size_t size_alerts = max_alerts * sizeof(alert);
    size_t mutex_size = sizeof(pthread_mutex_t);
    total_sensors_alerts_size = size_sensors +
                                size_sensor_indices +
                                size_active_sensors +
                                size_max_sensors +
                                size_alerts +
                                mutex_size;

    // map memory
    sensors_alerts_mapped_mem = mmap(NULL, total_sensors_alerts_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(sensors_alerts_mapped_mem == MAP_FAILED){
        printf("SHARED MEMORY INITIALIZATION FAILED");
        //homeiot_error_handler();
        exit(1);
    }

    // create sensors and alerts structure and assign the beginning
    // of each memory region to the sensors and to the alerts
    sensors_alerts sensors_alerts_shmem;
    sensors_alerts_shmem.sensors = (sensor_info*)sensors_alerts_mapped_mem;
    sensors_alerts_shmem.sensor_indices = (size_t*)(sensors_alerts_mapped_mem + size_sensors);
    sensors_alerts_shmem.alerts = (alert*)(sensors_alerts_mapped_mem +
                                            size_sensors +
                                            size_sensor_indices +
                                            size_active_sensors +
                                            size_max_sensors);

    sensors_alerts_shmem.max_sensors = max_sensors;
    // setting hash table with -1
    for(size_t i=0; i<max_sensors; i++){
        sensors_alerts_shmem.sensors[i].latest = -1;
        sensors_alerts_shmem.sensors[i].min = -1;
        sensors_alerts_shmem.sensors[i].max = -1;
        sensors_alerts_shmem.sensors[i].id[0] = '\0';
        sensors_alerts_shmem.sensors[i].key[0] = '\0';

        sensors_alerts_shmem.sensor_indices[i] = -1;
    }

    // init the mutex within the mapped memory
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&sensors_alerts_shmem.shmem_mutex, &attr);
    pthread_mutexattr_destroy(&attr);


    if((get_config_result = get_config_value("SENSOR_PIPE", &sensorFIFO, STRING)) != 1
        || sensorFIFO == NULL){
        if(get_config_result == 0) request_log_safe("ERROR", "SENSOR_PIPE config value type mismatch (STRING expected)");
        else if(get_config_result == -1) request_log_safe("ERROR", "SENSOR_PIPE config key not found");
        //home_iot_error_handler(); //TODO Implement this
    }

    /* SENSOR_PIPE creation */
    if(mkfifo(sensorFIFO, 0666) == -1){
        unlink(sensorFIFO);
        if(mkfifo(sensorFIFO, 0666) == -1){
            perror("mkfifo sensorFIFO");
            exit(1);
        }
    }

    if((sys_manager_pid = fork()) == 0){
        init_sys_manager(sensorFIFO, &sensors_alerts_shmem);
    }
#ifdef DEBUG
    printf("sys_manager_pid: %d\n", sys_manager_pid);
#endif

    waitpid(sys_manager_pid, 0, 0);
    waitpid(logger_pid, 0, 0);

    return 0;
};