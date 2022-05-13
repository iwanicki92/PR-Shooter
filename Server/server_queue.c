#include "server_queue.h"
#include "server_mutex.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct QueueNode {
    QueueNode* previous;
    QueueNode* next;
    void* data;    
};

void queueLock(SynchronizedQueue* queue) {
    lockMutex(queue->mutex);
}

void queueUnlock(SynchronizedQueue* queue) {
    unlockMutex(queue->mutex);
}

SynchronizedQueue queueSyncCreate(size_t sizeof_element) {
    SynchronizedQueue queue = { .first = NULL, .last = NULL,
                                .mutex = malloc(sizeof(pthread_mutex_t)), .element_size = sizeof_element };
    initRecursiveMutex(queue.mutex);
    return queue;
}

static QueueNode* createNewNode(size_t element_size, const void* element) {
    QueueNode* new_node = malloc(sizeof(QueueNode));
    new_node->data = malloc(element_size);
    memcpy(new_node->data, element, element_size);
    new_node->previous = NULL;
    new_node->next = NULL;
    return new_node;
}

void queueSyncPushBack(SynchronizedQueue* queue, const void* element) {
    queueLock(queue);
    QueueNode* new_node = createNewNode(queue->element_size, element);
    QueueNode* old_last = queue->last;
    new_node->previous = old_last;
    queue->last = new_node;
    if(old_last == NULL) {
        queue->first = new_node;
    } else {
        old_last->next = new_node;
    }
    queueUnlock(queue);
}

void queueSyncPushFront(SynchronizedQueue* queue, const void* element) {
    queueLock(queue);
    QueueNode* new_node = createNewNode(queue->element_size, element);
    QueueNode* old_first = queue->first;
    new_node->next = old_first;
    queue->first = new_node;
    if(old_first == NULL) {
        queue->last = new_node;
    } else {
        old_first->previous = new_node;
    }
    queueUnlock(queue);
}

static QueueNode* popFrontNode(SynchronizedQueue* queue) {
    queueLock(queue);
    assert((queue->first != NULL) && "Can't pop empty queue!");
    QueueNode* popped = queue->first;
    queue->first = popped->next;
    if(popped == queue->last) {
        queue->last = NULL;
    } else {
        popped->next->previous = NULL;
    }
    queueUnlock(queue);
    return popped;
}

void* queueSyncPopFront(SynchronizedQueue* queue) {
    QueueNode* front = popFrontNode(queue);
    void* data = front->data;
    free(front);
    return data;
}

void queueSyncPopCopyFront(SynchronizedQueue* queue, void* buf) {
    void* data = queueSyncPopFront(queue);
    memcpy(buf, data, queue->element_size);
    free(data);
}

bool queueIsEmpty(SynchronizedQueue* queue) {
    return queue->first == NULL;
}

void queueSyncDestroy(SynchronizedQueue* queue) {
    queueClear(queue);
    destroyMutex(queue->mutex);
    free(queue->mutex);
}

void queueClear(SynchronizedQueue* queue) {
    QueueNode* current = queue->first;
    while(current != NULL) {
        QueueNode* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    queue->first = queue->last = NULL;
}
