#include "sensor.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 128

int fd;
char* sensorFIFO;
char* sensorID;

void signal_dead_sensor(char* sensor_id){
    char staticBuffer[BUFFER_SIZE];
    // signal system that sensor is alive
    snprintf(staticBuffer, BUFFER_SIZE, "%c%s%cDEAD%c0%c",
             INFO_DELIMITER, sensor_id, VALUE_DELIMITER, VALUE_DELIMITER, VALUE_DELIMITER);
    write(fd, staticBuffer, strlen(staticBuffer));
}

void sensor_sigint_handler(){
    signal_dead_sensor(sensorID);
    unload_config_file();
    close(fd);
    exit(0);
}


void setup_sigint_signal(){
    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = sensor_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }
}


void validate_id(char* id){
    if((strlen(id) < 3 || strlen(id) > 32)){
        printf("INVALID SENSOR ID LENGTH\n");
        exit(1);
    }
    else{
        for(int i=0; i<strlen(id); i++){
            if(isalnum(id[i]) == 0){
                printf("INVALID SENSOR ID (CONTAINS NON ALPHANUMERIC CHARS)\n");
                exit(1);
            }
        }
    }
}


void validate_key(char* key){
    if((strlen(key) < 3 || strlen(key) > 32)){
        printf("KEY LENGTH INVALID\n");
        exit(1);
    }
    else{
        for(int i=0; i<strlen(key); i++){
            if(key[i] == '_');
            else if(isalnum(key[i]) == 0){
                printf("INVALID KEY (CONTAINS NON ALPHANUMERIC CHARS)\n");
                exit(1);
            }
        }

        if(strcmp(key, "DEAD")==0){
            printf("INVALID KEY (RESERVED)\n");
            exit(1);
        }
    }
}


void validate_fifo(){
    if(sensorFIFO == NULL){
        printf("SENSOR FIFO NOT DEFINED\n");
        exit(1);
    }
}


_Noreturn void simulate_sensor(char *sensor_id, int interval, char *key, int min, int max){
    if((fd = open(sensorFIFO, O_WRONLY)) < 0){ //Open sensor pipe to write data
        fprintf(stderr, "ERROR OPENING SENSOR PIPE FOR WRITING\n");
        exit(1);
    }

    size_t bufferSize = BUFFER_SIZE;
    char staticBuffer[bufferSize];
    char* dynamicBuffer;

    while(1){
        long val = (random() % (max - min + 1)) + min;

        int required_size = snprintf(NULL, 0, "%c%s%c%s%c%ld%c",
                                     INFO_DELIMITER, sensor_id, VALUE_DELIMITER, key, VALUE_DELIMITER, val, VALUE_DELIMITER) + 1;

        // if the required info string size exceeds that of the static buffer allocate dynamically
        if(required_size > BUFFER_SIZE){
            // allocate the buffer with the required size
            dynamicBuffer = malloc(required_size);
            // handle mem allocation error
            if (!dynamicBuffer) {
                perror("Failed to allocate memory for sensor_info");
                signal_dead_sensor(sensor_id);
                close(fd);
                exit(1);
            }

            // copy the sensor info to buffer
            snprintf(dynamicBuffer, required_size, "%c%s%c%s%c%ld%c",
                     INFO_DELIMITER, sensor_id, VALUE_DELIMITER, key, VALUE_DELIMITER, val, VALUE_DELIMITER);

            // write info to fifo
            ssize_t n = write(fd, dynamicBuffer, strlen(dynamicBuffer));
            // handle fifo write error
            if (n == -1) {
                perror("ERROR WRITING TO PIPE");
                free(dynamicBuffer);
                signal_dead_sensor(sensor_id);
                close(fd);
                exit(1);
            }
            // display sent info to fifo
            printf("INFO SENT: %s\n", dynamicBuffer);

            // deallocate buffer
            free(dynamicBuffer);

        } else { // if the required info string size is less or equal than that of the static buffer, use it
            // copy the sensor info to buffer
            snprintf(staticBuffer, required_size, "%c%s%c%s%c%ld%c",
                     INFO_DELIMITER, sensor_id, VALUE_DELIMITER, key, VALUE_DELIMITER, val, VALUE_DELIMITER);

            // write info to fifo
            ssize_t n = write(fd, staticBuffer, strlen(staticBuffer));
            // handle fifo write error
            if (n == -1) {
                perror("ERROR WRITING TO PIPE");
                signal_dead_sensor(sensor_id);
                close(fd);
                exit(1);
            }
            // display sent info to fifo
            printf("INFO SENT: %s\n", staticBuffer);
        }

        sleep(interval); //sleep for the provided interval before sending info
    }
}


int main(int argc, char* argv[]){

    if(argc != 7){
        if(argc < 7)
            fprintf(stdout, "Arguments missing!\nUsage: sensor *sensorID* *interval (seconds)* *key* *min value* *max value* *config file*\n");
        else
            fprintf(stdout, "Too many arguments!\n");
        exit(1);
    }

    setup_sigint_signal();

    int config_file_load_result = load_config_file(argv[6]);
    if(config_file_load_result != SUCCESS_CONFIG_FILE_LOAD){
        switch (config_file_load_result) {
            case ERROR_OPEN_CONFIG_FILE:
                fprintf(stderr, "UNABLE TO OPEN THE CONFIG FILE\n");
                break;
            case ERROR_ALLOCATE_BUFFER:
                fprintf(stderr, "ERROR ALLOCATING MEMORY");
                break;
            case ERROR_READ_CONFIG_FILE:
                fprintf(stderr, "ERROR READING FILE");
                break;
            case ERROR_PARSE_CONFIG_FILE:
                fprintf(stderr, "ERROR PARSING CONFIGURATION FILE\n");
                break;
            case ERROR_ALLOCATE_CONFIG_PARAMS:
                fprintf(stderr, "ERROR ALLOCATING MEMORY FOR CONFIG PARAMS\n");
                break;
            default:
                fprintf(stderr, "UNKNOWN CONFIGURATION ERROR OCCURED\n");
                break;
        }
        exit(1);
    }
    fprintf(stdout, "CONFIG FILE LOADED INTO MEMORY\n");

    int result;
    if((result = get_config_value("SENSOR_PIPE", &sensorFIFO, STRING)) != 1){
        if(result == 0) fprintf(stderr, "SENSOR_PIPE config value type mismatch (STRING expected)\n");
        else if(result == -1) fprintf(stderr, "SENSOR_PIPE config key not found\n");
        exit(1);
    }
    if(sensorFIFO == NULL){
        fprintf(stderr, "SENSOR_PIPE config value is NULL\n");
        exit(1);
    }
    fprintf(stdout, "Sensor FIFO location: %s\n", sensorFIFO);

    srand(time(0));

    sensorID = argv[1];
    int interval = atoi(argv[2]);
    char* key = argv[3];
    int min_value = atoi(argv[4]);
    int max_value = atoi(argv[5]);

    validate_id(sensorID);
    validate_key(key);

    simulate_sensor(sensorID, interval, key, min_value, max_value);
}