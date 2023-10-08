#include "log.h"
#include "string.h"
#include "ring_buffer.h"
#include "home_iot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#define DELIMITER "#"
#define SEPARATOR "|"

// time structure
time_t t;
struct tm* tm_info;

FILE* log_file;

temp_log_buffer_t temp_log_buffer;

void log_sigint_handler();
void log_error_handler();

char* get_current_time(){
    // get the current date
    time(&t);
    tm_info = localtime(&t);

    static char datetime[40];
    strftime(datetime, 40, "%d-%m-%Y %H:%M:%S", tm_info);
    return datetime;
}

int rbuffer_is_initialized(){
    if(ring_buffer_shmem == NULL){
        return 0;
    }
    return 1;
}

void request_log(char* type, char* message){
    // calculate length of the string to be written to ring buffer (type + 1 (| to separate) + message)
    size_t str_len = strlen(type) + strlen(message) + 1;
    char str[str_len];
    // join the string
    snprintf(str, str_len, "%s|%s", type, message);
    // write string to ring buffer
    put_ring(&ring_buffer_shmem->ring_buffer, str);
}

void request_log_safe(char* type, char* message){
    if(rbuffer_is_initialized()){
        // flush any logs from the temporary buffer first
        printf("temp_log_buffer.count: %zu\n", temp_log_buffer.count);
        for(size_t i = 0; i < temp_log_buffer.count; i++){
            request_log(temp_log_buffer.types[i], temp_log_buffer.messages[i]);
            free(temp_log_buffer.types[i]);
            free(temp_log_buffer.messages[i]);
        }
        //free(temp_log_buffer.types);
        //free(temp_log_buffer.messages);
        temp_log_buffer.count = 0;
        temp_log_buffer.capacity = 0;

        // log the current request to the ring buffer
        request_log(type, message);
    } else {
        // if temp log buffer is full, reallocate
        if(temp_log_buffer.count == temp_log_buffer.capacity){
            temp_log_buffer.capacity = (temp_log_buffer.capacity == 0) ? 64 : temp_log_buffer.capacity * 2;
            temp_log_buffer.types = realloc(temp_log_buffer.types, temp_log_buffer.capacity * sizeof(char*));
            temp_log_buffer.messages = realloc(temp_log_buffer.messages, temp_log_buffer.capacity * sizeof(char*));
            if(!temp_log_buffer.types || !temp_log_buffer.messages){
                printf("Error allocating memory\n");
                kill(getpid(), SIGINT);
                return;
            }
        }

        // store the log in the temp buffer
        temp_log_buffer.types[temp_log_buffer.count] = strdup(type);
        temp_log_buffer.messages[temp_log_buffer.count] = strdup(message);
        if (!temp_log_buffer.types[temp_log_buffer.count] || !temp_log_buffer.messages[temp_log_buffer.count]) {
            printf("Error allocating memory\n");
            kill(getpid(), SIGINT);
            return;
        }
        temp_log_buffer.count++;
    }
}

void write_log(char* type, char* message){
    char* datetime = get_current_time();
    int result = fprintf(log_file, "[%s] [%s] %s\n", datetime, type, message);
    if(result > 0){
        printf("LOG WRITTEN\n");
        return;
    }
    printf("ERROR - LOG NOT WRITTEN\n");
}

void process_remaining_logs(){
    int requests_count;
    // get semaphore value that represents the number of requests on the buffer
    sem_getvalue(&ring_buffer_shmem->ring_buffer.requests_count, &requests_count);

    // get x requests from the ring buffer
    for(int i=0; i<requests_count; i++){
        // get string from ring buffer
        char *extracted_string = get_ring(&ring_buffer_shmem->ring_buffer);
        // preserve original pointer to malloc'd extracted string for later freeing
        void* orig_string_ptr = extracted_string;

        // extract string before the | char (type)
        char *type = extract_string(extracted_string, SEPARATOR);
        if (type == NULL){
            printf("Couldn't extract string - skipping request\n");
            continue;
        }

        // point to string right after the | char (message)
        extracted_string = skip_delimiter(extracted_string, SEPARATOR);
        if (type == NULL){
            printf("Couldn't skip delimiter - skipping request\n");
            free(type);
            continue;
        }

        // write extracted strings to log file
        write_log(type, extracted_string);

        // free the malloc'd returned string by the extract_string()
        free(type);
        // free the original pointer of the extracted string from the ring buffer
        free(orig_string_ptr);
    }
    fprintf(log_file, "BYE BYE");
}

void log_sigint_handler(){
    request_log("INFO", "Logger process shutting down - SIGINT received");
    printf("Logger process shutting down - SIGINT received\n");
    process_remaining_logs();
    fflush(log_file);
    fclose(log_file);
    exit(0);
}

void log_error_handler(){
    request_log("ERROR", "Logger process shutting down - Internal Error");
    printf("Logger process shutting down - Internal Error\n");
    process_remaining_logs();
    fflush(log_file);
    fclose(log_file);
    kill(getppid(), SIGINT);
    exit(1);
}

char* generate_current_date(){
    // get the current date
    time(&t);
    tm_info = localtime(&t);

    static char datetime[20];
    strftime(datetime, 20, "%d-%m-%Y", tm_info);

    return datetime;
}

char* generate_log_name(){
    static char log_file_name[50];
    strcat(log_file_name, "home_iot.log.");

    //get current date
    char *date = generate_current_date();

    // concatenate log filename and date of creation
    strcat(log_file_name, date);

    return log_file_name;
}

_Noreturn void init_logger(){

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = log_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // register the sigint signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    char* log_file_name = generate_log_name();
    // open log file for writing
    log_file = fopen(log_file_name, "w");
    if(log_file == NULL){
        printf("ERROR OPENING LOG FILE\n");
        log_error_handler();
    }

    while(1){
        char *extracted_string = get_ring(&ring_buffer_shmem->ring_buffer);
        // preserve original pointer to malloc'd extracted string for later freeing
        void* orig_string_ptr = extracted_string;

        // extract string before the | char (type)
        char *type = extract_string(extracted_string, SEPARATOR);
        if (type == NULL){
            printf("Couldn't extract string - skipping request\n");
            continue;
        }

        // point to string right after the | char (message)
        extracted_string = skip_delimiter(extracted_string, SEPARATOR);
        if (type == NULL){
            printf("Couldn't skip delimiter - skipping request\n");
            free(type);
            continue;
        }

        // write extracted strings to log file
        write_log(type, extracted_string);

        // free the malloc'd returned string by the extract_string()
        free(type);
        // free the original pointer of the extracted string from the ring buffer
        free(orig_string_ptr);
    }
}