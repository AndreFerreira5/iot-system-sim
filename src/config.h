#ifndef IOT_SYSTEM_SIM_CONFIG_H
#define IOT_SYSTEM_SIM_CONFIG_H

#define SUCCESS_CONFIG_FILE_LOAD 0
#define ERROR_OPEN_CONFIG_FILE 1
#define ERROR_ALLOCATE_BUFFER 2
#define ERROR_READ_CONFIG_FILE 3
#define ERROR_PARSE_CONFIG_FILE 4
#define ERROR_ALLOCATE_CONFIG_PARAMS 5

typedef enum ValueType {
    STRING,
    INT
} ValueType;

struct ConfigParam{
    char* key;
    union {
        char *stringValue;
        int intValue;
    } value;
    ValueType valueType;
};

extern int entries_num;
extern struct ConfigParam* config_params_arr;

/* Loads the entries in the provided config file (JSON) into an array of structs */
int load_config_file(char* config_file);

/* Assigns the value of the provided key in case it exists returning 1.
 * If the expected value type differs from the actually value type returns 0.
 * If the provided key is not found returns -1 */
int get_config_value(const char* key, void* value, ValueType expectedType);

/* Deallocate the array of structs from memory */
void unload_config_file();

#endif //IOT_SYSTEM_SIM_CONFIG_H
