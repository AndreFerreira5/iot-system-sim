#include "log.h"
#include "string.h"
#include "ring_buffer.h"
#include "system_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define BUFFER_LEN 1024
#define DELIMITER "#"

// time structure
time_t t;
struct tm* tm_info;

// log pipe file descriptor for global access
int log_pipe_fd;

FILE* log_file;

void log_sigint_handler();
void error_handler();

char* get_current_time(){
    // get the current date
    time(&t);
    tm_info = localtime(&t);

    static char datetime[40];
    strftime(datetime, 40, "[%d-%m-%Y %H:%M:%S] ", tm_info);

    return datetime;
}

void request_log(char* type, char* message){
    int log_fd = open(LOG_PIPE, O_WRONLY);

    //if pipe fails to open
    if(log_fd == -1){
        perror("pipe open");
        return;
    }

    size_t log_bytes = strlen(type) + strlen(message) + 1 + 1 + 1; //strlen from type and message + # + # + \0

    char buffer[log_bytes];
    // assemble pipe message
    snprintf(buffer, log_bytes, "#%s#%s", type, message);
    //printf("SENT MSG: %s\n", buffer);
    // send message to pipe
    write(log_fd, buffer, log_bytes);
    // close pipe
    close(log_fd);
}

void write_log(char* type, char* message){
    char* datetime = get_current_time();
    fprintf(log_file, "%s [%s] %s\n", datetime, type, message);
    printf("LOG WRITTEN\n");
}

void process_remaining_logs(){
    int log_fd = open(LOG_PIPE, O_RDONLY | O_NONBLOCK);
    //if pipe fails to open
    if(log_fd == -1){
        perror("pipe open");
        return;
    }

    ssize_t bytes_read = 1;

    // read log requests until there are no more
    while(bytes_read > 0){
        char *buffer = (char*)malloc(BUFFER_LEN*sizeof(char));
        if(buffer == NULL){
            perror("malloc");
            return;
        }
        void* orig_buffer_ptr = buffer;
        bytes_read = read(log_pipe_fd, buffer, BUFFER_LEN);

        while(*buffer == '#'){
            buffer = skip_delimiter(buffer, DELIMITER);
            char* type = extract_string(buffer, DELIMITER);
            buffer = skip_delimiter(buffer, DELIMITER);

            write_log(type, buffer);
            free(type);
        }
        free(orig_buffer_ptr);
    }

    close(log_fd);
}

void log_sigint_handler(){
    close(log_pipe_fd);
    //request_log("INFO", "Logger process shutting down - SIGINT received");
    printf("Logger process shutting down - SIGINT received\n");
    //process_remaining_logs();
    fclose(log_file);
    exit(0);
}

void error_handler(){
    close(log_pipe_fd);
    //request_log("ERROR", "Logger process shutting down - Internal Error");
    printf("Logger process shutting down - Internal Error\n");
    //process_remaining_logs();
    fclose(log_file);
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

_Noreturn void init_logger(shared_ring_buffer *ring_buffer_shmem){

    // create sigaction struct
    struct sigaction sa;
    sa.sa_handler = log_sigint_handler;
    sigemptyset(&sa.sa_mask);
    //sa.sa_flags = 0;
    sigaddset(&sa.sa_mask, SIGINT);
    sigaction(SIGINT, &sa, NULL);

    // register the sigterm signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("ERROR REGISTERING SIGTERM SIGNAL HANDLER");
        exit(1);
    }

    char* log_file_name = generate_log_name();
    // open log file for writing
    log_file = fopen(log_file_name, "w");

    while(1){
        char* extracted_string = get_ring(&ring_buffer_shmem->ring_buffer);
        printf("READ\n");
        free(extracted_string);
    }





    // open log pipe for reading and writing to prevent read() to always return on EOF
    // effectively causing a busy wait
    log_pipe_fd = open(LOG_PIPE, O_RDWR);
    if(log_pipe_fd == -1){
        perror("pipe open");
        error_handler();
    }

    ssize_t num_read;
    while(1){

        char *buffer = (char*)malloc(BUFFER_LEN*sizeof(char));
        void* orig_buffer_ptr = buffer;
        num_read= read(log_pipe_fd, buffer, BUFFER_LEN);
        printf("READ: %zd\n", num_read);

        while(*buffer == '#'){
            buffer = skip_delimiter(buffer, DELIMITER);
            char* type = extract_string(buffer, DELIMITER);
            buffer = skip_delimiter(buffer, DELIMITER);

            write_log(type, buffer);
            free(type);
        }
        free(orig_buffer_ptr);

    }

}