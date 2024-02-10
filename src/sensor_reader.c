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
                   size_t staticBufferSize, size_t dynamicBufferSize,
                   char* staticBuffer, char* dynamicBuffer){

    ssize_t bytesReadCurrent=0, bytesReadTotal=0;

    // If the number of bytes read is the same as the buffer size, most likely there
    // is more info on the FIFO, so we need to get it as to not segment data between parsing cycles
    // To do that we use a dynamic buffer that will store all the additional info on the FIFO
    if(read(sensorPipeFD, staticBuffer, BUFFER_SIZE) == BUFFER_SIZE){
        fprintf(stderr, "READDDDD SENSOR READER\n");
        // While the number of bytes read is the same as the dynamic buffer size, there is more data
        // in the FIFO, so keep doubling the dynamic buffer in size and reading into it until it's
        // all read as to not segment the data
        while((bytesReadCurrent = read(sensorPipeFD, dynamicBuffer+bytesReadTotal, dynamicBufferSize-bytesReadTotal)) == dynamicBufferSize-bytesReadTotal){
            // handle FIFO reading error
            if(bytesReadTotal<0) {
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
        return DYNAMIC_READ;
    }
    fprintf(stderr, "READDDDD SENSOR READER\n");
    return STATIC_READ;
}

SensorData parse_sensor_info_to_node(char* staticBuffer, char value_delimiter[]){
    // Get values and check if each one is found
    // If one of them is not found, return a dummy node
    // After extracting the string delimited by the value_delimiter, skip it
    // as the extract_string doesn't do it itself
    char* sensorID = extract_string(staticBuffer, value_delimiter);
    if(sensorID == NULL) return (SensorData){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    char* sensorKey = extract_string(staticBuffer, value_delimiter);
    if(sensorKey == NULL) return (SensorData){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    char* sensorValue = extract_string(staticBuffer, value_delimiter);
    if(sensorValue == NULL) return (SensorData){'\0'};
    staticBuffer = skip_delimiter(staticBuffer, value_delimiter);

    // Create SensorData node
    SensorData node;
    strncpy(node.ID, sensorID, sizeof(node.ID) -1);
    node.ID[sizeof(node.ID) - 1] = '\0';
    strncpy(node.key, sensorKey, sizeof(node.key) -1);
    node.key[sizeof(node.key) - 1] = '\0';
    node.value = atol(sensorValue);

    free(sensorID);
    free(sensorKey);
    free(sensorValue);
    return node;
}

void parse_buffer_to_heap(char* staticBuffer, char info_delimiter[], char value_delimiter[], maxHeap* taskHeap){
    // Parse info while there is a INFO_DELIMITER (|)
    while((staticBuffer = find_delimiter(staticBuffer, info_delimiter)) != NULL){
        staticBuffer = skip_delimiter(staticBuffer, info_delimiter);
        SensorData node = parse_sensor_info_to_node(staticBuffer, value_delimiter);

        // Check if the node returned is valid
        // If not skip this iteration
        if(node.ID[0]=='\0') continue;

        // Insert node in Heap
        fprintf(stderr, "INSERTING IN HEAP\n");
        fprintf(stderr, "sensorID: %s - key: %s - value %ld\n", node.ID, node.key, node.value);
        insert_heap(taskHeap, TASK_PRIORITY, SENSOR_DATA, &node);
        fprintf(stderr, "INSERTED IN HEAP\n");
    }
}

_Noreturn void* init_sensor_reader(void* args){
    SensorReaderThreadArgs* threadArgs = (SensorReaderThreadArgs*)args;
    char* sensorFIFO = threadArgs->sensorFIFO;
    maxHeap* taskHeap = threadArgs->taskHeap;

    setup_sensor_reader_sigint_handler();

    int sensorPipeFD;
    if((sensorPipeFD = open(sensorFIFO, O_RDONLY)) < 0){ //Open sensor pipe to write data
        fprintf(stderr, "ERROR OPENING SENSOR PIPE FOR WRITING\n");
        request_log("ERROR", "ERROR OPENING SENSOR PIPE FOR WRITING");
        error_handler();
    }

    size_t staticBufferSize = BUFFER_SIZE;
    size_t dynamicBufferSize = BUFFER_SIZE;
    char staticBuffer[staticBufferSize];

    char info_delimiter[] = {INFO_DELIMITER, '\0'};
    char value_delimiter[] = {VALUE_DELIMITER, '\0'};
    while(1){
        char* dynamicBuffer = malloc(dynamicBufferSize);
        // handle allocation error
        if(!dynamicBuffer){
            request_log("ERROR", "Failed to allocate memory for dynamicBuffer");
            fprintf(stderr, "Failed to allocate memory for dynamicBuffer");
            error_handler();
        }

        fprintf(stderr, "ABOUT TO READ SENSOR READER\n");
        int readResult = read_from_fifo(sensorPipeFD, staticBufferSize, dynamicBufferSize, staticBuffer, dynamicBuffer);
        fprintf(stderr, "BUFFER: %s\n\n", staticBuffer);
        char* staticParsingBuffer = staticBuffer;
        if(readResult == STATIC_READ){
            fprintf(stderr, "STATIC READ\n");
            // parse static buffer
            parse_buffer_to_heap(staticParsingBuffer, info_delimiter, value_delimiter, taskHeap);
        } else if(readResult == DYNAMIC_READ){
            char* dynamicParsingBuffer = dynamicBuffer;
            // parse static buffer
            parse_buffer_to_heap(staticParsingBuffer, info_delimiter, value_delimiter, taskHeap);
            // parse dynamic buffer
            parse_buffer_to_heap(dynamicParsingBuffer, info_delimiter, value_delimiter, taskHeap);
        }
        free(dynamicBuffer);
    }
}