#include "server_array.h"
#include "server_mutex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void arrayLock(SynchronizedArray* array) {
#if (defined PRINT_DEBUG && PRINT_DEBUG > 1)
    printf("Locking array %p: [size: %ld, sizeof: %ld, capacity: %ld, mutex: %p\n",
            (void*)array, array->array.size, array->array.element_size, array->array.capacity, (void*)array->mutex);
#endif
    lockMutex(array->mutex);
}

void arrayUnlock(SynchronizedArray* array) {
#if (defined PRINT_DEBUG && PRINT_DEBUG > 1)
    printf("Unlocking array %p: [size: %ld, sizeof: %ld, capacity: %ld, mutex: %p\n",
            (void*)array, array->array.size, array->array.element_size, array->array.capacity, (void*)array->mutex);
#endif
    unlockMutex(array->mutex);
}

static void* getIndexAddress(Array* array, size_t index) {
    assert((index < array->capacity) && "Address outside of capacity");
    return (char*)array->ptr_array + index * array->element_size;
}

SynchronizedArray arraySyncCreate(size_t element_size, size_t starting_item_capacity) {
    SynchronizedArray syncArray = {.array = arrayCreate(element_size, starting_item_capacity),
                                    .mutex = malloc(sizeof(pthread_mutex_t))};
    initRecursiveMutex(syncArray.mutex);
    return syncArray;
}

Array arrayCreate(size_t element_size, size_t starting_item_capacity) {
    Array array = {.size = 0, .element_size = element_size,
                    .capacity = starting_item_capacity,
                    .ptr_array = malloc(array.element_size * array.capacity)};
    return array;
}

void resize(Array* array, size_t capacity) {
    if(array->capacity > capacity) {
        return;
    }
    void* new_ptr = realloc(array->ptr_array, capacity * array->element_size);
    if(new_ptr == NULL) {
        perror("realloc() error!");
        exit(1);
    }
    array->ptr_array = new_ptr;
    array->capacity = capacity;
}

size_t arraySyncPushBack(SynchronizedArray* array,  const void* item) {
    arrayLock(array);
#if (defined PRINT_DEBUG && PRINT_DEBUG > 2)
    printf("Sync array push back\n");
#endif
    size_t index = arrayPushBack(&array->array, item);
    arrayUnlock(array);
    return index;
}

size_t arrayPushBack(Array* array, const void* item) {
    size_t index = array->size;
    if(index >= array->capacity) {
        resize(array, array->capacity * 2);
    }
    memcpy(getIndexAddress(array, index), item, array->element_size);
    ++(array->size);
    return index;
}

void arraySyncDestroy(SynchronizedArray* array) {
    arrayDestroy(&array->array);
    destroyMutex(array->mutex);
    free(array->mutex);
}

void arrayDestroy(Array* array) {
    free(array->ptr_array);
    array->ptr_array = NULL;
}

size_t arraySyncSize(SynchronizedArray* array) {
    arrayLock(array);
#if (defined PRINT_DEBUG && PRINT_DEBUG > 2)
    printf("Array size\n");
#endif
    size_t size = arraySize(&array->array);
    arrayUnlock(array);
    return size;
}

size_t arraySize(Array* array) {
    return array->size;
}

void arraySyncGetItem(SynchronizedArray* array, size_t index, void* buf) {
    arrayLock(array);
#if (defined PRINT_DEBUG && PRINT_DEBUG > 2)
    printf("Array get item\n");
#endif
    arrayGetItem(&array->array, index, buf);
    arrayUnlock(array);
}

void arrayGetItem(Array* array, size_t index, void* buf) {
    memcpy(buf, getIndexAddress(array, index), array->element_size);
}

void* arraySyncUnsafeGetItem(SynchronizedArray* array, size_t index) {
    return arrayUnsafeGetItem(&array->array, index);
}

void* arrayUnsafeGetItem(Array* array, size_t index) {
    return getIndexAddress(array, index);
}

void arraySyncClear(SynchronizedArray* array) {
    arrayLock(array);
    arrayClear(&array->array);
    arrayUnlock(array);
}

void arrayClear(Array* array) {
    array->size = 0;
}

Array arrayGetCopy(Array* array) {
    Array new_array = arrayCreate(array->element_size, array->size);
    new_array.size = array->size;
    memcpy(new_array.ptr_array, array->ptr_array, array->size * array->element_size);
    return new_array;
}
