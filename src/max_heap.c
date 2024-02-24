#include "max_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

maxHeap* create_heap(int capacity){
    // allocate shared memory for maxHeap structure
    maxHeap* heap = mmap(NULL, sizeof(maxHeap), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (heap == MAP_FAILED) {
        fprintf(stderr, "mmap for maxHeap structure failed");
        return NULL;
    }

    // allocate shared memory for the heap array within the maxHeap structure
    heap->heap = mmap(NULL, capacity * sizeof(node), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (heap->heap == MAP_FAILED) {
        fprintf(stderr, "mmap for heap array failed");
        munmap(heap, sizeof(maxHeap));  // clean up previously allocated memory
        return NULL;
    }

    // init mutex attribute
    pthread_mutexattr_init(&heap->heapMutexAttr);
    pthread_mutexattr_settype(&heap->heapMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutexattr_setpshared(&heap->heapMutexAttr, PTHREAD_PROCESS_SHARED);
    // init mutex with attribute
    pthread_mutex_init(&heap->heapMutex, &heap->heapMutexAttr);

    // destroy the mutex attribute as it is no longer needed
    pthread_mutexattr_destroy(&heap->heapMutexAttr);

    // init the semaphore as shared between processes
    if (sem_init(&heap->tasksSem, 1, 0) != 0) {
        fprintf(stderr, "sem_init failed");
        munmap(heap->heap, capacity * sizeof(node));  // clean up previously allocated memory
        munmap(heap, sizeof(maxHeap));
        return NULL;
    }

    heap->size = 0;
    heap->capacity = capacity;

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
    if(maxHeap->size == maxHeap->capacity){
        fprintf(stderr, "HEAP FULL - DROPPING TASK!\n");
        pthread_mutex_unlock(&maxHeap->heapMutex);
        return;
    }

    // insert node at the end
    size_t i = maxHeap->size++;
    maxHeap->heap[i].priority = priority;
    maxHeap->heap[i].type = type;
    switch(type){
        case(SENSOR_DATA):
            maxHeap->heap[i].sensor = *(sensor*)data;
            break;
        case USER_CONSOLE_DATA:
            maxHeap->heap[i].userConsoleData = *(UserConsoleData*)data;
            break;
        default:
            // revert changes and return
            maxHeap->size--;
            pthread_mutex_unlock(&maxHeap->heapMutex);
            return;
    }

    // fix the max heap if violated
    while(i!=0 && maxHeap->heap[(i -1) / 2].priority < maxHeap->heap[i].priority){
        swap(&maxHeap->heap[i], &maxHeap->heap[(i -1) / 2]);
        i = (i -1) / 2;
    }

    // Unlock Heap mutex
    pthread_mutex_unlock(&maxHeap->heapMutex);

    // post task semaphore, adding 1 to its value
    // signaling a new avaliable task on the heap
    sem_post(&maxHeap->tasksSem);

    int sem_value;
    sem_getvalue(&maxHeap->tasksSem, &sem_value);
    fprintf(stderr, "MAX HEAP SEMAPHORE VALUE: %d\n", sem_value);
}

node extract_max(maxHeap* maxHeap){

    // wait for the task semaphore (if its value is 0
    // it means there is no task in the heap, therefore
    // blocking the process until there is one)
    sem_wait(&maxHeap->tasksSem);

    // Lock Heap mutex
    pthread_mutex_lock(&maxHeap->heapMutex);

    if(maxHeap->size <= 0) {
        pthread_mutex_unlock(&maxHeap->heapMutex);
        return (node) {-1, INVALID}; // return dummy node
    }
    if(maxHeap->size == 1){
        maxHeap->size--;
        pthread_mutex_unlock(&maxHeap->heapMutex);
        return maxHeap->heap[0];
    }

    node root = maxHeap->heap[0];
    maxHeap->heap[0] = maxHeap->heap[--maxHeap->size];
    heapify(maxHeap, 0);

    // Unlock Heap mutex
    pthread_mutex_unlock(&maxHeap->heapMutex);

    return root;
}

void unmap_heap(maxHeap* maxHeap){
    if(!maxHeap) return;

    // lock heap mutex
    pthread_mutex_lock(&maxHeap->heapMutex);

    sem_destroy(&maxHeap->tasksSem);

    if (maxHeap->heap) {
        munmap(maxHeap->heap, maxHeap->capacity * sizeof(node));
    }

    // unlock the mutex before destroying it
    pthread_mutex_unlock(&maxHeap->heapMutex);

    // destroy heap mutex
    pthread_mutex_destroy(&maxHeap->heapMutex);

    // unmap the maxheap struct
    munmap(maxHeap, sizeof(maxHeap));
}