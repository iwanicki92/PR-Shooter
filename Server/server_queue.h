#ifndef SERVER_QUEUE_H
#define SERVER_QUEUE_H
#include <stdbool.h>
#include <pthread.h>

struct QueueNode;
typedef struct QueueNode QueueNode;

// Each queue created with queueSyncCreate needs to be destroyed by queueSyncDestroy
typedef struct {
    QueueNode* first;
    QueueNode* last;
    pthread_mutex_t* mutex;
    const size_t element_size;
} SynchronizedQueue;

// locks queue for other threads until unlocked
void queueLock(SynchronizedQueue* queue);
void queueUnlock(SynchronizedQueue* queue);
SynchronizedQueue queueSyncCreate(size_t sizeof_element);
// adds new copy of *element to queue
void queueSyncPushBack(SynchronizedQueue* queue, const void* element);
// removes and returns first element(first one pushed). User needs to free returned pointer after use
void* queueSyncPopFront(SynchronizedQueue* queue);
// removes first element after copyping it into *buf
void queueSyncPopCopyFront(SynchronizedQueue* queue, void* buf);
bool queueIsEmpty(SynchronizedQueue* queue);
// not thread safe! Only one thread can operate on queue during this function.
// after destroying queue can't be used before initializing again(by creating new)
void queueSyncDestroy(SynchronizedQueue* queue);
void queueClear(SynchronizedQueue* queue);

#endif