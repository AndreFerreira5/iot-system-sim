#include "home_iot.h"
#include "log.h"
#include "config.h"
#include "system_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <sys/mman.h>
#include "worker.h"

pid_t sys_manager_pid, logger_pid;
shared_ring_buffer *ring_buffer_shmem;
size_t rbuffer_shmem_size;

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

    load_config_file(argv[1]);
  
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

    if((sys_manager_pid = fork()) == 0){
        init_sys_manager();
    }
    printf("sys_manager_pid: %d\n", sys_manager_pid);

    waitpid(sys_manager_pid, 0, 0);
    waitpid(logger_pid, 0, 0);

    return 0;
};