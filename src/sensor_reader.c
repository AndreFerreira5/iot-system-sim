#include "sensor_reader.h"
#include "system_manager.h"
#include "string.h"
#include "sensor.h"
#include "log.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define STATIC_READ 0
#define DYNAMIC_READ 1

_Noreturn void error_handler(){
    raise(SIGINT);
    pthread_exit(NULL);
}

_Noreturn void sigint_handler(){
    raise(SIGINT);
    pthread_exit(NULL);
}

void setup_sensor_reader_sigint_handler(){
    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        request_log("ERROR", "Couldn't register SIGINT signal handler");
        fprintf(stderr, "ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        error_handler();
    }
}

int read_from_fifo(int sensorPipeFD,
                   ssize_t staticBufferSize, ssize_t dynamicBufferSize,
                   char* staticBuffer, char* dynamicBuffer){

    ssize_t bytesReadCurrent=0, bytesReadTotal=0;

    
    // If the number of bytes read is the same as the buffer size, most likely there
    // is more info on the FIFO, so we need to get it as to not segment data between parsing cycles
    // To do that we use a dynamic buffer that will store all the additional info on the FIFO
#ifdef DEBUG
    ssize_t bytesRead = 0;
    if((bytesRead = read(sensorPipeFD, staticBuffer, staticBufferSize)) == staticBufferSize){
        fprintf(stdout, "[SENSOR READER] Read %zd bytes from SENSOR FIFO, exceeded static buffer size, using dynamic buffer!\n", bytesRead);
#else
    if(read(sensorPipeFD, staticBuffer, staticBufferSize) == staticBufferSize){
#endif

        dynamicBuffer = malloc(dynamicBufferSize);
        if(!dynamicBuffer) {
            request_log("ERROR", "Failed to allocate memory for dynamicBuffer");
            fprintf(stderr, "Failed to allocate memory for dynamicBuffer");
            error_handler();
        }
        memcpy(dynamicBuffer, staticBuffer, staticBufferSize);
        bytesReadTotal += staticBufferSize;

        // While the number of bytes read is the same as the dynamic buffer size, there is more data
        // in the FIFO, so keep doubling the dynamic buffer in size and reading into it until it's
        // all read as to not segment the data
        while((bytesReadCurrent = read(sensorPipeFD, dynamicBuffer+bytesReadTotal, dynamicBufferSize-bytesReadTotal)) == dynamicBufferSize-bytesReadTotal){
            // handle FIFO reading error
            if(bytesReadCurrent<0) {
                request_log("ERROR", "FIFO reading error (returned < 0)");
                fprintf(stderr, "FIFO reading error (returned < 0)");
                error_handler();
            }
            // update total bytes read
            bytesReadTotal += bytesReadCurrent;

            // double the dynamic buffer in size and reallocate it
            dynamicBufferSize *= 2;
            dynamicBuffer = realloc(dynamicBuffer, dynamicBufferSize);
            // handle realloc error
            if(!dynamicBuffer){
                request_log("ERROR", "Failed to reallocate memory for dynamicBuffer");
                fprintf(stderr, "Failed to reallocate memory for dynamicBuffer");
                error_handler();
            }
        }
#ifdef DEBUG
        fprintf(stdout, "[SENSOR READER] Read %zd bytes from SENSOR FIFO, using dynamic buffer!\n", bytesReadTotal);
        fprintf(stdout, "%s\n", dynamicBuffer);
#endif
        return DYNAMIC_READ;
    }

#ifdef DEBUG
    fprintf(stdout, "[SENSOR READER] Read %zd bytes from SENSOR FIFO, using static buffer!\n", bytesRead);
#endif

    return STATIC_READ;
}

sensor parse_sensor_info_to_node(char* staticBuffer, char value_delimiter[]){
    // Get values and check if each one is found
    // If one of them is not found, return a dummy node
    // After extracting the string delimited by the value_delimiter, skip it
    // as the extract_string doesn't do it itself
    char* sensorID = extract_string(staticBuffer, value_delimiter);
    if(sensorID == NULL) return (sensor){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    char* sensorKey = extract_string(staticBuffer, value_delimiter);
    if(sensorKey == NULL) return (sensor){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    char* sensorValue = extract_string(staticBuffer, value_delimiter);
    if(sensorValue == NULL) return (sensor){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    // Create SensorData node
    sensor node;
    strncpy(node.id, sensorID, sizeof(node.id) -1);
    node.id[sizeof(node.id) - 1] = '\0';
    strncpy(node.key, sensorKey, sizeof(node.key) -1);
    node.key[sizeof(node.key) - 1] = '\0';
    node.value = atol(sensorValue);

    free(sensorID);
    free(sensorKey);
    free(sensorValue);
    return node;
}

void parse_buffer_to_heap(char* buffer, char info_delimiter[], char value_delimiter[], maxHeap* taskHeap){
    // Parse info while there is a INFO_DELIMITER (|)
    while((buffer = find_delimiter(buffer, info_delimiter)) != NULL){
        buffer = skip_delimiter(buffer, info_delimiter);
        sensor node = parse_sensor_info_to_node(buffer, value_delimiter);

        // Check if the node returned is valid
        // If not skip this iteration
        if(node.id[0]=='\0') continue;

        // Insert node in Heap
#ifdef DEBUG
        fprintf(stderr, "INSERTING IN HEAP\n");
        fprintf(stderr, "SensorID: %s - Key: %s - Value: %ld\n", node.id, node.key, node.value);
#endif
        insert_heap(taskHeap, TASK_PRIORITY, SENSOR_DATA, &node);
    }
}

_Noreturn void* init_sensor_reader(void* args){
    SensorReaderThreadArgs* threadArgs = (SensorReaderThreadArgs*)args;
    char* sensorFIFO = threadArgs->sensorFIFO;
    maxHeap* taskHeap = threadArgs->taskHeap;

    setup_sensor_reader_sigint_handler();

    int sensorPipeFD;
    if((sensorPipeFD = open(sensorFIFO, O_RDWR)) < 0){ //Open sensor pipe to write data
        fprintf(stderr, "ERROR OPENING SENSOR PIPE FOR WRITING\n");
        request_log("ERROR", "ERROR OPENING SENSOR PIPE FOR WRITING");
        error_handler();
    }

    ssize_t staticBufferSize = BUFFER_SIZE;
    ssize_t dynamicBufferSize = BUFFER_SIZE*2; // dynamic buffer size is double than that of the static buffer, so, in case
                                              // the static buffer is not enough, its content is copied to the dynamic buffer,
                                              // effectively occupying BUFFER_SIZE while still having BUFFER_SIZE free
                                              // (and then growing exponentially, as needed)
    char staticBuffer[staticBufferSize];
    char* dynamicBuffer = NULL;

    char info_delimiter[] = {INFO_DELIMITER, '\0'};
    char value_delimiter[] = {VALUE_DELIMITER, '\0'};
    while(1){

        int readResult = read_from_fifo(sensorPipeFD, staticBufferSize, dynamicBufferSize, staticBuffer, dynamicBuffer);
        if(readResult == STATIC_READ){
            // parse static buffer
            char* staticParsingBuffer = staticBuffer;
            parse_buffer_to_heap(staticParsingBuffer, info_delimiter, value_delimiter, taskHeap);
        } else if(readResult == DYNAMIC_READ){
            char* dynamicParsingBuffer = dynamicBuffer;
            // parse dynamic buffer
            parse_buffer_to_heap(dynamicParsingBuffer, info_delimiter, value_delimiter, taskHeap);

            free(dynamicBuffer);
            dynamicBuffer = NULL;
        }
    }
}
