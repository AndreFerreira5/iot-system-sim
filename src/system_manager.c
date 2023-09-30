#include "system_manager.h"
#include "queue.h"
#include "log.h"
#include "ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <sys/mman.h>

pid_t logger_pid;
shared_ring_buffer *ring_buffer_shmem;
size_t rbuffer_shmem_size;

void sys_sigint_handler(){

    //request_log("INFO", "System Manager shutting down - SIGINT received");
    printf("System Manager shutting down - SIGINT received\n");

    //request_log("INFO", "Waiting for logger process to shutdown");
    printf("WAITING FOR LOGGER PROCESS TO SHUTDOWN\n");

    // close logger process as last for all the logs to be logged
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

    exit(0);
}

void init_sys_manager(){

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sys_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigterm signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    // create shared memory for ring buffer
    rbuffer_shmem_size = sizeof(ring_buffer_t);
    ring_buffer_shmem = mmap(NULL, rbuffer_shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(ring_buffer_shmem == MAP_FAILED){
        printf("SHARED MEMORY INITIALIZATION FAILED");
        //TODO error handling
    }

    // create ring buffer
    ring_buffer_shmem->ring_buffer = create_ring_buffer();

    // create logger process
    if ((logger_pid = fork()) == 0){
        init_logger(ring_buffer_shmem);
    }
    printf("logger_pid: %d\n", logger_pid);

    // set internal queue size
    set_queue_size();

    // create internal queue to hold tasks (change this)
    Task *INTERNAL_QUEUE;

    sleep(1);
    put_ring(&ring_buffer_shmem->ring_buffer, "this is a string!");
    printf("ring_buffer: %s\n", ring_buffer_shmem->ring_buffer.buffer);
    sleep(1);
    put_ring(&ring_buffer_shmem->ring_buffer, "this is another string!");
    printf("ring_buffer: %s\n", ring_buffer_shmem->ring_buffer.buffer);

    waitpid(logger_pid, 0, 0);
}