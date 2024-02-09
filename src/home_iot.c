#include "home_iot.h"
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
char* sensorFIFO;
int sensorFIFODesc;

void home_sigint_handler(){

    request_log("INFO", "Home_IoT shutting down - SIGINT received");
    printf("Home_IoT shutting down - SIGINT received\n");

    /* SYS MANAGER */
    request_log("INFO", "Waiting for system manager process to shutdown");
    printf("WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // send SIGINT to the system manager process
    if(kill(sys_manager_pid, SIGINT) == -1){
        perror("kill sigint");
    }

    //wait for the system manager process to finish
    waitpid(sys_manager_pid, 0, 0);


    request_log("INFO", "Waiting for logger process to shutdown");
    printf("WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // close the logger process as last for all the logs to be logged
    if(kill(logger_pid, SIGINT) == -1){
        perror("kill sigint");
    }

    //wait for logger process to finish
    waitpid(logger_pid, 0, 0);
    printf("logger process shuted down\n");

    // destroy ring buffer semaphores
    sem_destroy(&ring_buffer_shmem->ring_buffer.ring_buffer_sem);
    sem_destroy(&ring_buffer_shmem->ring_buffer.requests_count);

    // unmap shared memory
    munmap(ring_buffer_shmem, rbuffer_shmem_size);

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
    printf("logger_pid: %d\n", logger_pid);

    // create ring buffer
    ring_buffer_shmem->ring_buffer = create_ring_buffer();

    request_log_safe("INFO", "HOME IOT BOOTING UP");

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
        printf("logger process shuted down\n");

        exit(1);
    }
    request_log_safe("INFO", "CONFIG FILE LOADED INTO MEMORY");
    fprintf(stdout, "CONFIG FILE LOADED INTO MEMORY\n");

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

    int get_config_result;
    if((get_config_result = get_config_value("SENSOR_PIPE", &sensorFIFO, STRING)) != 1
        || sensorFIFO == NULL){
        if(get_config_result == 0) request_log_safe("ERROR", "SENSOR_PIPE config value type mismatch (STRING expected)");
        else if(get_config_result == -1) request_log_safe("ERROR", "SENSOR_PIPE config key not found");
        //home_iot_error_handler(); //TODO Implement this
    }

    /* SENSOR_PIPE creation */
    if(mkfifo(sensorFIFO, 0666) == -1){
        perror("mkfifo sensorFIFO");
        exit(1);
    }

    if((sys_manager_pid = fork()) == 0){
        init_sys_manager();
    }
    printf("sys_manager_pid: %d\n", sys_manager_pid);

    waitpid(sys_manager_pid, 0, 0);
    waitpid(logger_pid, 0, 0);

    return 0;
};