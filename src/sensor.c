#include "sensor.h"
#include "shared_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

int fd;
char* sensorFIFO;

void sensor_sigint_handler(){
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
        printf("ERROR OPENING SENSOR PIPE FOR WRITING (SENSOR PROCESS)\n");
        exit(1);
    }
    while(1){
        int val = (rand() % (max - min + 1)) + min;
        char *sensor_info = malloc(strlen(sensor_id) + strlen(key) + strlen("#") + sizeof(int) + 1);
        sprintf(sensor_info, "#%s#%s#%d", sensor_id, key, val);
        printf("INFO SENT: %s\n", sensor_info);

        int n = write(fd,sensor_info, strlen(sensor_info));
        if(n == -1){
            printf("ERROR WRITING TO PIPE\n");
            close(fd);
            free(sensor_info);
            free(sensor_id);
            free(key);
            exit(1);
        }
        free(sensor_info);
        sleep(interval); //sleep for the provided interval before sending info
    }
}


int main(int argc, char* argv[]){

    if(argc != 6){
        if(argc < 6)
            fprintf(stdout, "Arguments missing!\nUsage: sensor *sensorID* *interval (seconds)* *key* *min value* *max value*\n");
        else
            fprintf(stdout, "Too many arguments!\n");
        exit(1);
    }


    srand(time(0));

    char* sensorID = argv[1];
    int interval = atoi(argv[2]);
    char* key = argv[3];
    int min_value = atoi(argv[4]);
    int max_value = atoi(argv[5]);

    validate_id(sensorID);
    validate_key(key);
    validate_fifo();

    simulate_sensor(sensorID, interval, key, min_value, max_value);
}