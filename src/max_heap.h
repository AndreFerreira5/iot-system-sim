#ifndef IOT_SYSTEM_SIM_MAX_HEAP_H
#define IOT_SYSTEM_SIM_MAX_HEAP_H

#include <stdlib.h>

typedef struct Node{
    u_int8_t priority;
    char* data;
} node;

typedef struct MaxHeap{
    node* heap;
    int size;
    int capacity;
    pthread_mutex_t heapMutex;
} maxHeap;

maxHeap* create_heap(int capacity);
void insert_heap(maxHeap* maxHeap, int priority, char* data);
node extract_max(maxHeap* maxHeap);
void free_heap(maxHeap* maxHeap);

#endif //IOT_SYSTEM_SIM_MAX_HEAP_H
