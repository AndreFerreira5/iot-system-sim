#include "home_iot.h"
#include "system_manager.h"
#include "log.h"
#include "config.h"
#include "worker.h"
#include "sensor_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>

maxHeap* taskHeap;

void sys_sigint_handler(){

    request_log("INFO", "System Manager shutting down - SIGINT received");
    printf("System Manager shutting down - SIGINT received\n");

    // if taskheap is mapped on memory, unmap it
    if(taskHeap) unmap_heap(taskHeap);

    // propagate sigint upwards
    kill(getppid(), SIGINT);

    exit(0);
}

void sys_error_handler(){
    request_log("INFO", "System Manager shutting down - Internal Error");
    printf("System Manager shutting down - Internal Error\n");

    // if taskheap is mapped on memory, unmap it
    if(taskHeap) unmap_heap(taskHeap);

    // propagate sigint upwards
    kill(getppid(), SIGINT);
    exit(1);
}

void setup_sigint_handler(){
    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sys_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        request_log("ERROR", "Couldn't register SIGINT signal handler");
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        sys_error_handler();
    }
}

void init_sys_manager(char* sensorFIFO, sensors_alerts* sensors_alerts_shmem){
    request_log("INFO", "[SYSTEM MANAGER] BOOTING UP");

    setup_sigint_handler();

    /* Task Heap creation */
    int heap_capacity = 0;
    int result;
    if((result = get_config_value("HEAP_CAPACITY", &heap_capacity, INT)) != 1){ // if there's an error
        if(result == 0) request_log("ERROR", "HEAP_CAPACITY config value type mismatch (INT expected)");
        else if(result == -1) request_log("ERROR", "HEAP_CAPACITY config key not found");
        sys_error_handler();
    }
    if(heap_capacity <= 0){ // if the config value is invalid, exit
        request_log("ERROR", "HEAP_CAPACITY config value invalid (must be bigger than 0)");
        fprintf(stderr, "HEAP_CAPACITY config value invalid (must be bigger than 0)");
        sys_error_handler();
    }
    taskHeap = create_heap(heap_capacity);
    if(taskHeap == NULL){
        request_log("ERROR", "Error creating shared memory for Task Heap");
        fprintf(stderr, "Error creating shared memory for Task Heap");
        sys_error_handler();
    }

    // Sensor reader thread arguments creation
    SensorReaderThreadArgs sensorReaderThreadArgs;
    sensorReaderThreadArgs.sensorFIFO = sensorFIFO;
    sensorReaderThreadArgs.taskHeap = taskHeap;


    /* Sensor Reader thread creation */
    pthread_t sensor_reader_id;
    pthread_create(&sensor_reader_id, NULL, init_sensor_reader, &sensorReaderThreadArgs);


    /* Worker processes spawning */
    size_t num_workers = 0;
    if((result = get_config_value("NUM_WORKERS", &num_workers, INT)) != 1){
        if(result == 0) request_log("ERROR", "NUM_WORKERS config value type mismatch (INT expected)");
        else if(result == -1) request_log("ERROR", "NUM_WORKERS config key not found");
        sys_error_handler();
    }
    if(num_workers <= 0){
        request_log("ERROR", "NUM_WORKERS config value invalid (must be bigger than 0)");
        sys_error_handler();
    }
    pid_t workers_pid[num_workers];
    for(size_t i=0; i<num_workers; i++) {
        if ((workers_pid[i] = fork()) == 0) {
            init_worker(sensors_alerts_shmem, taskHeap);
        }
    }

    // wait for sensor reader thread
    pthread_join(sensor_reader_id, NULL);

    // wait for all worker processes
    for(size_t i=0; i<num_workers; i++) {
        waitpid(workers_pid[i], 0, 0);
    }

    exit(0);
}