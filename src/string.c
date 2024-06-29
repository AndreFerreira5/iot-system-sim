#include "string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* find_delimiter(char* start, const char* delimiter) {
    if (start == NULL || delimiter == NULL || *delimiter == '\0') {
        return NULL;
    }

    size_t delimiter_len = strlen(delimiter);

    if (delimiter_len == 1) {
        // single character delimiter case
        char delim = *delimiter;
        while (*start != '\0') {
            if (*start == delim) {
                return start;
            }
            start++;
        }
    } else {
        // general case for multi-character delimiters
        while (*start != '\0') {
            if (strncmp(start, delimiter, delimiter_len) == 0) {
                return start;
            }
            start++;
        }
    }
    return NULL;
}

char* extract_string(char* start, const char* delimiter) {
    if (start == NULL || delimiter == NULL || *delimiter == '\0') {
        return NULL;
    }

    char* end = find_delimiter(start, delimiter);
    if (end) {
        size_t len = end - start;
        char* str = (char*)malloc(len + 1);
        if (str == NULL) {
            fprintf(stderr, "Failed to allocate memory");
            return NULL;
        }
        memcpy(str, start, len);
        str[len] = '\0';
        return str;
    }
    return NULL;
}

char* skip_delimiter(char* start, const char* delimiter) {
    if (start == NULL || delimiter == NULL || *delimiter == '\0') {
        return NULL;
    }

    size_t delimiter_len = strlen(delimiter);

    if (delimiter_len == 1) {
        // single character delimiter case
        char delim = *delimiter;
        while (*start != '\0') {
            if (*start == delim) {
                return start + 1;
            }
            start++;
        }
    } else {
        // general case for multi-character delimiters
        while (*start != '\0') {
            if (strncmp(start, delimiter, delimiter_len) == 0) {
                return start + delimiter_len;
            }
            start++;
        }
    }
    return NULL;
}
