#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON/cJSON.h>

struct ConfigParam* config_params_arr;
int entries_num;

int load_config_file(char* config_file){
    FILE *fp;
    char *buffer;
    long file_size;

    // open the configuration file for reading
    fp = fopen(config_file, "r");
    if (fp == NULL){
        return ERROR_OPEN_CONFIG_FILE;
    }

    // get the configuration file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate memory for the buffer
    buffer = (char*)malloc(file_size + 1); // +1 for null temrinator
    if(buffer == NULL){
        fclose(fp);
        return ERROR_ALLOCATE_BUFFER;
    }

    if(fread(buffer, 1, file_size, fp) != file_size){
        fclose(fp);
        free(buffer);
        return ERROR_READ_CONFIG_FILE;
    }

    // null-terminate the buffer
    buffer[file_size] = '\0';
    fclose(fp);


    // parse the JSON data
    cJSON *config_json = cJSON_Parse(buffer);
    free(buffer); // free the allocated buffer
    if (config_json == NULL){
        cJSON_Delete(config_json);
        return ERROR_PARSE_CONFIG_FILE;
    }

    // get number of configuration keys
    entries_num = cJSON_GetArraySize(config_json);

    // allocate correct amount of memory for the array of structs
    config_params_arr = (struct ConfigParam*)malloc(entries_num * sizeof(struct ConfigParam));
    if(config_params_arr == NULL){
        cJSON_Delete(config_json);
        return ERROR_ALLOCATE_CONFIG_PARAMS;
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
    return SUCCESS_CONFIG_FILE_LOAD;
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
