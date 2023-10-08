#include "string.h"
#include <stdlib.h>
#include <string.h>

char* find_delimiter(char* start, const char* delimiter){
    while(*start != '\0'){
        if(*start == *delimiter){
            return start;
        }
        start++;
    }
    return NULL;
}

char* extract_string(char* start, const char* delimiter){
    char* end = find_delimiter(start, delimiter);
    if(end){
        size_t len = end - start;
        char* str = malloc(len + 1);
        memcpy(str, start, len);
        str[len] = '\0';
        return str;
    }
    return NULL;
}

char* skip_delimiter(char* start, const char* delimiter){
    while(*start != '\0'){
        if(*start == *delimiter){
            start++;
            return start;
        }
        start++;
    }
    return NULL;
}