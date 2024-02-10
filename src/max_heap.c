#include "max_heap.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

maxHeap* create_heap(int capacity){
    maxHeap* heap = (maxHeap*) malloc(sizeof(maxHeap));
    if(!heap){
        return NULL;
    }

    pthread_mutex_init(&heap->heapMutex, NULL);
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

void insert_heap(maxHeap* maxHeap, int priority, char* data){
    // if heap is full, don't insert
    if(maxHeap->size == maxHeap->capacity) return;

    // insert node at the end
    size_t i = maxHeap->size++;
    maxHeap->heap[i].priority = priority;
    maxHeap->heap[i].data = strdup(data);

    // fix the max heap if violated
    while(i!=0 && maxHeap->heap[(i -1) / 2].priority < maxHeap->heap[i].priority){
        swap(&maxHeap->heap[i], &maxHeap->heap[(i -1) / 2]);
        i = (i -1) / 2;
    }
}

node extract_max(maxHeap* maxHeap){
    if(maxHeap->size <= 0)
        return (node){-1, NULL}; // return dummy node
    if(maxHeap->size == 1){
        maxHeap->size--;
        return maxHeap->heap[0];
    }

    node root = maxHeap->heap[0];
    maxHeap->heap[0] = maxHeap->heap[--maxHeap->size];
    heapify(maxHeap, 0);

    return root;
}

void free_heap(maxHeap* maxHeap){
    // free all nodes data
    for(size_t i=0; i < maxHeap->size; i++)
        free(maxHeap->heap[i].data);

    free(maxHeap->heap);
    free(maxHeap);
}