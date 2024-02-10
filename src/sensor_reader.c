#include "sensor_reader.h"
#include "system_manager.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

void error_handler(){

}

_Noreturn void* init_sensor_reader(void* args){
    SensorReaderThreadArgs* threadArgs = (SensorReaderThreadArgs*)args;
    char* sensorFIFO = threadArgs->sensorFIFO;
    maxHeap* taskHeap = threadArgs->taskHeap;

    int sensorPipeFD;
    if((sensorPipeFD = open(sensorFIFO, O_WRONLY)) < 0){ //Open sensor pipe to write data
        fprintf(stderr, "ERROR OPENING SENSOR PIPE FOR WRITING\n");
        error_handler();
    }

    size_t bufferSize = BUFFER_SIZE;
    char buffer[bufferSize];
    char* dynamicBuffer;
    ssize_t bytesRead;
    while(1){
        //bytesRead = read(sensorPipeFD, buffer, );
    }
}