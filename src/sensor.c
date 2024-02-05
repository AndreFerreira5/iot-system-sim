#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){

    if(argc != 6){
        if(argc < 6)
            fprintf(stdout, "Arguments missing!\nUsage: sensor *sensorID* *interval (seconds)* *key* *min value* *max value*\n");
        else
            fprintf(stdout, "Too many arguments!\n");
        exit(1);
    }


}