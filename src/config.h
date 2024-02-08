#ifndef IOT_SYSTEM_SIM_CONFIG_H
#define IOT_SYSTEM_SIM_CONFIG_H

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
void load_config_file(char* config_file);

/* Assigns the value of the provided key in case it exists returning 1.
 * If the expected value type differs from the actually value type returns 0.
 * If the provided key is not found returns -1 */
int get_config_value(const char* key, void* value, ValueType expectedType);

/* Deallocate the array of structs from memory */
void unload_config_file();

#endif //IOT_SYSTEM_SIM_CONFIG_H
