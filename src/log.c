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
#include <stdarg.h>

#define MAX_BUFFER_SIZE 4096
#define SEPARATOR_STR "|"
#define SEPARATOR_CHAR '|'


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
    return ring_buffer_shmem != NULL;
}


void request_log(char* type, const char* format, ...){
    va_list args;
    va_start(args, format);

   // calculate the formatted string length
    size_t msg_len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    size_t type_len = strlen(type);
    // calculate length of the string to be written to ring buffer (type + 1 (| to separate) + message)
    size_t total_len = type_len + 1 + msg_len;

    // if the string passed is empty, return
    if(total_len <= 1) return;

    // if the log length exceeds the max buffer size allocate memory on the heap
    if(total_len >= MAX_BUFFER_SIZE){
        // allocate buffer with additional byte for null terminator
        char* buffer = (char*)malloc(total_len + 1);
        // if the memory allocation fails just drop log request
        if(!buffer) return;

        // construct string
        snprintf(buffer, total_len, "%s%c", type, SEPARATOR_CHAR);
        va_start(args, format);
        vsnprintf(buffer + type_len + 1, msg_len, format, args);
        va_end(args);

        // write string to ring buffer
        put_ring(&ring_buffer_shmem->ring_buffer, buffer);

        // free allocated buffer
        free(buffer);
    } else { // if the log length is less than the MAX_BUFFER_SIZE
        // create static buffer with calculated string length
        char buffer[total_len];

        // construct string
        snprintf(buffer, total_len, "%s%c", type, SEPARATOR_CHAR);
        va_start(args, format);
        vsnprintf(buffer + type_len + 1, msg_len, format, args);
        va_end(args);

        // write string to ring buffer
        put_ring(&ring_buffer_shmem->ring_buffer, buffer);
    }
}


void flush_temp_log_buffer(){
    // flush any logs from the temporary buffer first
    for(size_t i = 0; i < temp_log_buffer.count; i++){
        request_log(temp_log_buffer.types[i], temp_log_buffer.messages[i]);
        free(temp_log_buffer.types[i]);
        free(temp_log_buffer.messages[i]);
    }
    free(temp_log_buffer.types);
    free(temp_log_buffer.messages);
    temp_log_buffer.count = 0;
    temp_log_buffer.capacity = 0;
}


void expand_temp_log_buffer(){
    temp_log_buffer.capacity = (temp_log_buffer.capacity == 0) ? 64 : temp_log_buffer.capacity * 2;
    temp_log_buffer.types = realloc(temp_log_buffer.types, temp_log_buffer.capacity * sizeof(char*));
    temp_log_buffer.messages = realloc(temp_log_buffer.messages, temp_log_buffer.capacity * sizeof(char*));
    if(!temp_log_buffer.types || !temp_log_buffer.messages){
        fprintf(stderr, "Error allocating memory for temp_log_buffer\n");
        kill(getpid(), SIGINT);
        return;
    }
}


void request_log_safe(char* type, const char* format, ...){
    if(rbuffer_is_initialized()){

        if(temp_log_buffer.count>0)
            flush_temp_log_buffer();

        // log the current request to the ring buffer
        va_list args;
        va_start(args, format);
        // get formatted message size + 1 for null terminator
        size_t formatted_message_size = vsnprintf(NULL, 0, format, args) + 1;
        va_end(args);

        // allocate memory for the formatted message buffer
        char* formatted_message = (char*)malloc(formatted_message_size);
        // if the memory allocation errors, just drop the log
        if(!formatted_message) return;
        va_start(args, format);
        vsnprintf(formatted_message, formatted_message_size, format, args);
        va_end(args);
        request_log(type, formatted_message);
        va_end(args);
    } else {
        // if temp log buffer is full, reallocate
        if(temp_log_buffer.count == temp_log_buffer.capacity){
            expand_temp_log_buffer();
        }

        va_list args;
        va_start(args, format);
        // get formatted message size + 1 for null terminator
        size_t formatted_message_size = vsnprintf(NULL, 0, format, args) + 1;
        va_end(args);

        // allocate memory for the formatted message buffer
        char* formatted_message = (char*)malloc(formatted_message_size);
        // if the memory allocation errors, just drop the log
        if(!formatted_message) return;
        va_start(args, format);
        vsnprintf(formatted_message, formatted_message_size, format, args);
        va_end(args);

        // store the log in the temp buffer
        temp_log_buffer.types[temp_log_buffer.count] = strdup(type);
        temp_log_buffer.messages[temp_log_buffer.count] = formatted_message;
        // if the memory allocation errors, just drop the log
        if (!temp_log_buffer.types[temp_log_buffer.count]) {
            free(formatted_message);
            return;
        }
        temp_log_buffer.count++;
    }
}

void write_log(char* type, char* message){
    char* datetime = get_current_time();
    int result = fprintf(log_file, "[%s] [%s] %s\n", datetime, type, message);
    if(result > 0){
        fprintf(stdout, "LOG WRITTEN\n");
        return;
    }
    fprintf(stderr, "ERROR - LOG NOT WRITTEN\n");
}

void process_remaining_logs(){
    int requests_count;
    // get semaphore value that represents the number of requests on the buffer
    sem_getvalue(&ring_buffer_shmem->ring_buffer.requests_count, &requests_count);
    fprintf(stdout, "Remaining logs to process: %d\n", requests_count);
    // get x requests from the ring buffer
    for(int i=0; i<requests_count; i++){
        // get string from ring buffer
        char *extracted_string = get_ring(&ring_buffer_shmem->ring_buffer);
        // preserve original pointer to malloc'd extracted string for later freeing
        void* orig_string_ptr = extracted_string;

        // extract string before the | char (type)
        char *type = extract_string(extracted_string, SEPARATOR_STR);
        if (type == NULL){
            fprintf(stderr, "Couldn't extract string - skipping request\n");
            continue;
        }

        // point to string right after the | char (message)
        extracted_string = skip_delimiter(extracted_string, SEPARATOR_STR);
        if (type == NULL){
            fprintf(stderr, "Couldn't skip delimiter - skipping request\n");
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
    fprintf(stdout, "Logger process shutting down - SIGINT received\n");
    process_remaining_logs();
    fflush(log_file);
    fclose(log_file);
    exit(0);
}

void log_error_handler(){
    request_log("ERROR", "Logger process shutting down - Internal Error");
    fprintf(stdout, "Logger process shutting down - Internal Error\n");
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
        fprintf(stderr, "ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    char* log_file_name = generate_log_name();
    // open log file for writing
    log_file = fopen(log_file_name, "w");
    if(log_file == NULL){
        fprintf(stderr, "ERROR OPENING LOG FILE\n");
        log_error_handler();
    }

    while(1){
        char *extracted_string = get_ring(&ring_buffer_shmem->ring_buffer);
        // preserve original pointer to malloc'd extracted string for later freeing
        void* orig_string_ptr = extracted_string;

        // extract string before the | char (type)
        char *type = extract_string(extracted_string, SEPARATOR_STR);
        if (type == NULL){
            fprintf(stderr, "Couldn't extract string - skipping request\n");
            continue;
        }

        // point to string right after the | char (message)
        extracted_string = skip_delimiter(extracted_string, SEPARATOR_STR);
        if (type == NULL){
            fprintf(stderr, "Couldn't skip delimiter - skipping request\n");
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