#ifndef IOT_SYSTEM_SIM_RING_BUFFER_H
#define IOT_SYSTEM_SIM_RING_BUFFER_H
#include <pthread.h>
#include <semaphore.h>
#define MAX_BUFFER_SIZE 4096

typedef struct{
    char buffer[MAX_BUFFER_SIZE];
    size_t head;
    size_t tail;
    sem_t ring_buffer_sem, requests_count;
}ring_buffer_t;

/* Creates and returns an initialized ring buffer structure */
ring_buffer_t create_ring_buffer();

/* Writes provided string to provided ring buffer */
void put_ring(ring_buffer_t *buffer, char *string);

/* Blocking function that waits for a string to be written to the provided ring buffer and returns it */
char* get_ring(ring_buffer_t *ring_buffer);

#endif //IOT_SYSTEM_SIM_RING_BUFFER_H
