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
        request_log_safe("ERROR", "Unable to open config file");
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
        request_log_safe("ERROR", "Couldn't allocate memory for buffer");
        printf("ERROR ALLOCATING MEMORY");
        exit(1);
    }

    if(fread(buffer, 1, file_size, fp) != file_size){
        fclose(fp);
        free(buffer);
        request_log_safe("ERROR", "Couldn't read config file");
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
        request_log_safe("ERROR", "Couldn't parse config file");
        printf("ERROR PARSING CONFIGURATION FILE\n");
        const char* error_ptr = cJSON_GetErrorPtr();
        if(error_ptr != NULL){
            request_log_safe("ERROR", (char*)error_ptr);
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
        request_log_safe("ERROR", "Couldn't allocate memory for config parameters");
        printf("ERROR ALLOCATING MEMORY FOR CONFIG PARAMS\n");

        // close program
        exit(1);
    }

    for(int i=0; i<entries_num; i++){
        cJSON *item = cJSON_GetArrayItem(config_json, i);
        if(item->string){
            config_params_arr[i].key = strdup(item->string);
        }

        if(cJSON_IsString(item)){
            config_params_arr[i].value.stringValue = strdup(item->valuestring);
            config_params_arr[i].valueType = STRING;
        } else if (cJSON_IsNumber(item)) {
            config_params_arr[i].value.intValue = item->valueint;
            config_params_arr[i].valueType = INT;
        }
    }

    // delete the JSON object
    cJSON_Delete(config_json);
    request_log_safe("INFO", "CONFIG FILE LOADED INTO MEMORY");
}


int get_config_value(const char* key, void* value, ValueType expectedType){
    for(size_t i=0; i<entries_num; i++){
        if(strcmp(config_params_arr[i].key, key) == 0){ // if key matches
            printf("key: %s", config_params_arr[i].key);
            // if the expected type is int and the key value is int
            if(expectedType==INT && config_params_arr->valueType==INT){
                // assign int to provided value
                *(int*)value = config_params_arr[i].value.intValue;
                return 1; // flag success
            }
            // if the expected type is string and the key value is string
            else if(expectedType==STRING && config_params_arr[i].valueType==STRING){
                // assign string pointer to provided value
                printf("\t value: %s", config_params_arr[i].value.stringValue);
                *(char**)value = config_params_arr[i].value.stringValue;
                return 1; // flag success
            } else {
                return 0; // flag type mismatch
            }
        }
    }
    return -1; // flag key not found
}


void unload_config_file(){
    for(size_t i=0; i<entries_num; i++){
        // if the value type is string and it exists free it
        if(config_params_arr[i].valueType==STRING && config_params_arr[i].value.stringValue!=NULL)
            free(config_params_arr[i].value.stringValue);
        // if the key exists free it
        if(config_params_arr[i].key!=NULL)
            free(config_params_arr[i].key);
    }
    // free the struct array
    free(config_params_arr);
}
