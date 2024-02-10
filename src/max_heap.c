#include "max_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

maxHeap* create_heap(int capacity){
    maxHeap* heap = (maxHeap*) malloc(sizeof(maxHeap));
    if(!heap){
        return NULL;
    }
    pthread_mutexattr_init(&heap->heapMutexAttr);
    pthread_mutexattr_settype(&heap->heapMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&heap->heapMutex, &heap->heapMutexAttr);
    pthread_mutexattr_destroy(&heap->heapMutexAttr);

    heap->size = 0;
    heap->capacity = capacity;
    heap->heap = (node*)malloc(capacity * sizeof(node));

    return heap;
}

void swap(node* a, node* b){
    node temp = *a;
    *a = *b;
    *b = temp;
}

void heapify(maxHeap * maxHeap, size_t idx) {
    size_t largest = idx;
    size_t left = 2 * idx + 1;
    size_t right = 2 * idx + 2;

    if (left < maxHeap->size && maxHeap->heap[left].priority > maxHeap->heap[largest].priority)
        largest = left;

    if (right < maxHeap->size && maxHeap->heap[right].priority > maxHeap->heap[largest].priority)
        largest = right;

    if (largest != idx) {
        swap(&maxHeap->heap[idx], &maxHeap->heap[largest]);
        heapify(maxHeap, largest);
    }
}

void insert_heap(maxHeap* maxHeap, int priority, DataType type, void* data){
    // Lock Heap mutex
    fprintf(stderr, "WAITING FOR MUTEX\n");
    pthread_mutex_lock(&maxHeap->heapMutex);
    fprintf(stderr, "MUTEX UNLOCKED\n");

    // if heap is full, don't insert
    if(maxHeap->size == maxHeap->capacity) return;

    // insert node at the end
    size_t i = maxHeap->size++;
    maxHeap->heap[i].priority = priority;
    maxHeap->heap[i].type = type;
    switch(type){
        case(SENSOR_DATA):
            maxHeap->heap[i].sensorData = *(SensorData*)data;
            break;
        case USER_CONSOLE_DATA:
            maxHeap->heap[i].userConsoleData = *(UserConsoleData*)data;
            break;
        default:
            // revert changes and return
            maxHeap->size--;
            return;
    }

    // fix the max heap if violated
    while(i!=0 && maxHeap->heap[(i -1) / 2].priority < maxHeap->heap[i].priority){
        swap(&maxHeap->heap[i], &maxHeap->heap[(i -1) / 2]);
        i = (i -1) / 2;
    }

    // Unlock Heap mutex
    pthread_mutex_unlock(&maxHeap->heapMutex);
}

node extract_max(maxHeap* maxHeap){
    // Lock Heap mutex
    pthread_mutex_lock(&maxHeap->heapMutex);

    if(maxHeap->size <= 0)
        return (node){-1, INVALID}; // return dummy node
    if(maxHeap->size == 1){
        maxHeap->size--;
        return maxHeap->heap[0];
    }

    node root = maxHeap->heap[0];
    maxHeap->heap[0] = maxHeap->heap[--maxHeap->size];
    heapify(maxHeap, 0);

    // Unlock Heap mutex
    pthread_mutex_unlock(&maxHeap->heapMutex);

    return root;
}

void free_heap(maxHeap* maxHeap){
    // Lock Heap mutex
    pthread_mutex_lock(&maxHeap->heapMutex);

    free(maxHeap->heap);

    // Destroy Heap mutex
    pthread_mutex_destroy(&maxHeap->heapMutex);

    free(maxHeap);
}