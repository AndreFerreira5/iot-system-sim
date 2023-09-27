#ifndef IOT_SYSTEM_SIM_STRING_H
#define IOT_SYSTEM_SIM_STRING_H

/* Search mapped region for provided delimiter char returning it if found, otherwise returning NULL*/
char* find_delimiter(char* start, const char* delimiter);

/* Extract substring from start to provided delimiter returning it if found, otherwise returning NULL*/
char* extract_string(char* start, const char* delimiter);

/* Skip past delimiter char in mapped region*/
char* skip_delimiter(char* start, const char* delimiter);

#endif //IOT_SYSTEM_SIM_STRING_H
