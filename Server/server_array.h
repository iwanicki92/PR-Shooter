#ifndef SERVER_ARRAY_H
#define SERVER_ARRAY_H
#include <stddef.h>
#include <pthread.h>

typedef void (*Consumer)(void*);

typedef struct {
    // number of items
    size_t size;
    // size of item in bytes
    size_t sizeof_item;
    // total item capacity of ptr_array. Or capacity * sizeof_item bytes;
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
SynchronizedArray arraySyncGetNewArray(size_t sizeof_item, size_t starting_item_capacity);
Array arrayGetNewArray(size_t sizeof_item, size_t starting_item_capacity);
// copies array.sizeof_item bytes from item into array.ptr_array starting at past the last element(array + size * sizeof_item)
// returns index of pushed element
size_t arraySyncPushBack(SynchronizedArray* array, void* item);
size_t arrayPushBack(Array* array, void* item);
// not thread safe! Only one thread can operate on Array during this function.
// after destroying Array can't be used before initializing again
void arraySyncDestroy(SynchronizedArray* array);
void arrayDestroy(Array* array);
size_t arraySyncSize(SynchronizedArray* array);
size_t arraySize(Array* array);
// copies item to buf, buf needs to be at least array->sizeof_item bytes big
void arraySyncGetItem(SynchronizedArray* array, size_t index, void* buf);
void arrayGetItem(Array* array, size_t index, void* buf);
// use only between lockArray and unlockArray!
// returns pointer to item
void* arraySyncUnsafeGetItem(SynchronizedArray* array, size_t index);
// returns pointer to item.
void* arrayUnsafeGetItem(Array* array, size_t index);
void arraySyncClear(SynchronizedArray* array);
void arrayClear(Array* array);

#endif