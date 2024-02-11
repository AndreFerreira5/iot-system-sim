#include "ring_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DELIMITER '#'

char delimiter = DELIMITER;

ring_buffer_t create_ring_buffer(){
    ring_buffer_t  rbuffer;

    /* setting head and tail pointers to 0 */
    rbuffer.head = 0;
    rbuffer.tail = 0;

    // initializing ring buffer semaphore to 1 for only 1 concurrent access
    sem_init(&rbuffer.ring_buffer_sem, 1, 1);
    // initializing request count to 0 for the writer to post it (+1) when it finishes writing a request to it
    // and for the reader to wait it (-1) before it reads the request, essentially blocking the reader when
    // there is no requests to read
    sem_init(&rbuffer.requests_count, 1, 0);

    // clearing the buffer
    memset(rbuffer.buffer, 0, MAX_BUFFER_SIZE);

    // returning the ring buffer structure initialized
    return rbuffer;
}

void put_ring(ring_buffer_t *rbuffer, char *string){
    // get string length
    size_t str_len = strlen(string);
    // acquire buffer
    sem_wait(&rbuffer->ring_buffer_sem);
    // copy each char from provided string to ring buffer at the correct position
    for(int i=0; i<str_len; i++){
        // calculate ring buffer offset to write char from provided string in the correct position
        size_t buffer_offset = (rbuffer->head + i) % MAX_BUFFER_SIZE;
        // write char to buffer starting position + previously calculated buffer offset
        *(rbuffer->buffer + buffer_offset) = *(string+i);
    }

    // write # at the end of the ring buffer to signal end of string
    *(rbuffer->buffer + ((rbuffer->head + str_len) % MAX_BUFFER_SIZE)) = delimiter;

    // update ring buffer head (increment length of copied string + 1 for the #)
    rbuffer->head = (rbuffer->head + str_len + 1) % MAX_BUFFER_SIZE;
    //rbuffer->head += str_len+1 % MAX_BUFFER_SIZE;

    // posting semaphore so reader (logger process) knows there is one more request on the buffer
    sem_post(&rbuffer->requests_count);
    // release buffer
    sem_post(&rbuffer->ring_buffer_sem);
}

char* get_ring(ring_buffer_t *rbuffer){
    // check if there are requests on the buffer, if not wait (for a sem_post from the writer)
    sem_wait(&rbuffer->requests_count);


    /* calculate the string length by iterating until # (end of string) is found */
    size_t str_len = 0;
    int i = 0;

    // calculating the length of the string to read (until # is found)
    size_t buffer_offset = (rbuffer->tail + i) % MAX_BUFFER_SIZE;
    while(*(rbuffer->buffer + buffer_offset) != delimiter){
        str_len++;
        i++;
        buffer_offset = (rbuffer->tail + i) % MAX_BUFFER_SIZE;
    }
    str_len++; // for the '\0'

    // allocate memory for extracted string with previously calculated length
    char *extracted_string = (char*)malloc(str_len*sizeof(char));
    // copy char by char from ring buffer to malloc'd string
    for(i=0; i<str_len; i++){
        buffer_offset = (rbuffer->tail + i) % MAX_BUFFER_SIZE;
        extracted_string[i] = *(rbuffer->buffer + buffer_offset);
    }
    // set string terminator at the # position
    extracted_string[str_len-1] = '\0';

    // update ring buffer tail by incrementing it with string length (and the #)
    // and wrapping around if end is reached
    rbuffer->tail = (rbuffer->tail + str_len) % MAX_BUFFER_SIZE;

    // finally return the string extracted from the ring buffer
    return extracted_string;
}