#ifndef IOT_SYSTEM_SIM_MAX_HEAP_H
#define IOT_SYSTEM_SIM_MAX_HEAP_H

#include <stdlib.h>
#include <semaphore.h>

typedef enum {
    SENSOR_DATA,
    USER_CONSOLE_DATA,
    INVALID
} DataType;


typedef struct{
    char id[33];
    char key[33];
    long value;
} sensor;

typedef struct{
    // TODO finish this struct
} UserConsoleData;

typedef struct Node{
    u_int8_t priority;
    DataType type;
    union {
        sensor sensor;
        UserConsoleData userConsoleData;
    };
} node;

typedef struct MaxHeap{
    node* heap;
    int size;
    int capacity;
    pthread_mutexattr_t heapMutexAttr;
    pthread_mutex_t heapMutex;
    sem_t tasksSem;
} maxHeap;

maxHeap* create_heap(int capacity);
void insert_heap(maxHeap* maxHeap, int priority, DataType type, void* data);
node extract_max(maxHeap* maxHeap);
void unmap_heap(maxHeap* maxHeap);

#endif //IOT_SYSTEM_SIM_MAX_HEAP_H
