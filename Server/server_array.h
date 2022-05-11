#ifndef SERVER_ARRAY_H
#define SERVER_ARRAY_H
#include <stddef.h>
#include <pthread.h>

typedef void (*Consumer)(void*);

// Each array or SynchronizedArray created with queueCreate or queueSyncCreate
// needs to be destroyed by queueDestroy or queueSyncDestroy
typedef struct {
    // number of items
    size_t size;
    // size of item in bytes
    size_t element_size;
    // total item capacity of ptr_array. Or capacity * element_size bytes;
    size_t capacity;
    void* ptr_array;
} Array;

// don't use members directly in multi-threaded application
typedef struct {
    Array array;
    pthread_mutex_t* mutex;
} SynchronizedArray;

// locks array for other threads until unlocked
void arrayLock(SynchronizedArray* array);
void arrayUnlock(SynchronizedArray* array);
SynchronizedArray arraySyncCreate(size_t element_size, size_t starting_item_capacity);
Array arrayCreate(size_t element_size, size_t starting_item_capacity);
// copies array.element_size bytes from item into array.ptr_array starting at past the last element(array + size * element_size)
// returns index of pushed element
size_t arraySyncPushBack(SynchronizedArray* array, const void* item);
size_t arrayPushBack(Array* array, const void* item);
// not thread safe! Only one thread can operate on Array during this function.
// after destroying Array can't be used before initializing again
void arraySyncDestroy(SynchronizedArray* array);
void arrayDestroy(Array* array);
size_t arraySyncSize(SynchronizedArray* array);
size_t arraySize(Array* array);
// copies item to buf, buf needs to be at least array->element_size bytes big
void arraySyncGetItem(SynchronizedArray* array, size_t index, void* buf);
void arrayGetItem(Array* array, size_t index, void* buf);
// use only between lockArray and unlockArray!
// returns pointer to item
void* arraySyncUnsafeGetItem(SynchronizedArray* array, size_t index);
// returns pointer to item.
void* arrayUnsafeGetItem(Array* array, size_t index);
void arraySyncClear(SynchronizedArray* array);
void arrayClear(Array* array);
// remember to destroy your copy after use!
Array arrayGetCopy(Array* array);

#endif
