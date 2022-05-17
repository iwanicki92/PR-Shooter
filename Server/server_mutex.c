#include "server_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void setAttribute(pthread_mutexattr_t* attr, int type) {
    if ((errno = pthread_mutexattr_init(attr)) != 0) {
        perror("pthread_mutexattr_init() error");
        exit(1);
    }
    if((errno = pthread_mutexattr_settype(attr, type)) != 0) {
        perror("pthread_mutexattr_settype() error");
        exit(1);
    }
}

void initMutexWithAttribute(pthread_mutex_t* mutex, int type) {
    pthread_mutexattr_t attr;
    setAttribute(&attr, type);
    if((errno = pthread_mutex_init(mutex, &attr)) != 0) {
        perror("pthread_mutex_init() error");
        exit(1);
    }
}

void initMutex(pthread_mutex_t* mutex) {
    initMutexWithAttribute(mutex, PTHREAD_MUTEX_ERRORCHECK);
}

void initRecursiveMutex(pthread_mutex_t* mutex) {
    initMutexWithAttribute(mutex, PTHREAD_MUTEX_RECURSIVE);
}

void destroyMutex(pthread_mutex_t* mutex) {
    if((errno = pthread_mutex_destroy(mutex)) != 0) {
        perror("pthread_mutex_destroy() error");
        exit(1);
    }
}

void lockMutex(pthread_mutex_t* mutex) {
    //#if (defined PRINT_DEBUG && PRINT_DEBUG > 0)
    //printf("lockMutex:[%p], Thread: [%ld]\n", (void*)mutex, pthread_self());
    //fflush(stdout);
    //#endif
    if((errno = pthread_mutex_lock(mutex)) != 0) {
        perror("pthread_mutex_lock() error");
        exit(1);
    }
}

void unlockMutex(pthread_mutex_t* mutex) {
    #if (defined PRINT_DEBUG && PRINT_DEBUG > 0)
    printf("unlockMutex:[%p], Thread: [%ld]\n", (void*)mutex, pthread_self());
    fflush(stdout);
    #endif
    if((errno = pthread_mutex_unlock(mutex)) != 0) {
        perror("pthread_mutex_unlock() error");
        exit(1);
    }
}
