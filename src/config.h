#ifndef IOT_SYSTEM_SIM_CONFIG_H
#define IOT_SYSTEM_SIM_CONFIG_H

struct ConfigParam{
    const char* key;
    int value;
};

extern int entries_num;
extern struct ConfigParam* config_params_arr;

/* Loads the entries in the provided config file (JSON) into an array of structs */
void load_config_file(char* config_file);

/* Returns the value of the provided entry in case it exists, otherwise returns 0 */
int get_config_value(char* entry);

/* Deallocate the array of structs from memory */
void unload_config_file();

#endif //IOT_SYSTEM_SIM_CONFIG_H
