#include "config.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON/cJSON.h>

struct ConfigParam* config_params_arr;
int entries_num;

void load_config_file(char* config_file){
    FILE *fp;
    char *buffer;
    long file_size;

    // open the configuration file for reading
    fp = fopen(config_file, "r");
    if (fp == NULL){
        request_log("ERROR", "Unable to open config file");
        printf("UNABLE TO OPEN THE CONFIG FILE\n");

        // close program
        exit(1);
    }

    // get the configuration file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate memory for the buffer
    buffer = (char*)malloc(file_size + 1); // +1 for null temrinator
    if(buffer == NULL){
        fclose(fp);
        request_log("ERROR", "Couldn't allocate memory for buffer");
        printf("ERROR ALLOCATING MEMORY");
        exit(1);
    }

    if(fread(buffer, 1, file_size, fp) != file_size){
        fclose(fp);
        free(buffer);
        request_log("ERROR", "Couldn't read config file");
        printf("ERROR READING FILE");

        //close program
        exit(1);
    }

    // null-terminate the buffer
    buffer[file_size] = '\0';
    fclose(fp);


    // parse the JSON data
    cJSON *config_json = cJSON_Parse(buffer);
    free(buffer); // free the allocated buffer
    if (config_json == NULL){
        request_log("ERROR", "Couldn't parse config file");
        printf("ERROR PARSING CONFIGURATION FILE\n");
        const char* error_ptr = cJSON_GetErrorPtr();
        if(error_ptr != NULL){
            request_log("ERROR", (char*)error_ptr);
            printf("ERROR: %s\n", error_ptr);
        }
        cJSON_Delete(config_json);


        // close program
        exit(1);
    }

    // get number of configuration keys
    entries_num = cJSON_GetArraySize(config_json);

    // allocate correct amount of memory for the array of structs
    config_params_arr = (struct ConfigParam*)malloc(entries_num * sizeof(struct ConfigParam));
    if(config_params_arr == NULL){
        cJSON_Delete(config_json);
        request_log("ERROR", "Couldn't allocate memory for config parameters");
        printf("ERROR ALLOCATING MEMORY FOR CONFIG PARAMS\n");

        // close program
        exit(1);
    }

    // point to first config entry
    config_json = config_json->child;
    for(int i=0; i<entries_num; i++){
        config_params_arr[i].key = config_json->string;
        config_params_arr[i].value = config_json->valueint;
        config_json = config_json->next;
    }

    // delete the JSON object
    cJSON_Delete(config_json);
    //request_log("INFO", "CONFIG FILE LOADED INTO MEMORY");
}


int get_config_value(char* entry){
    // iterate through the array
    for(int i=0; i<entries_num; i++){
        // if entry is found
        if(strcmp(config_params_arr[i].key, entry) == 0){
            // return entry's value
            return config_params_arr[i].value;
        }
    }
    // if entry is not found, return 0
    return 0;
}


void unload_config_file(){
    free(config_params_arr);
}
