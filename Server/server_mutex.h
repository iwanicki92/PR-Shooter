#ifndef SERVER_MUTEX_H
#define SERVER_MUTEX_H
#include <pthread.h>

void initMutex(pthread_mutex_t* mutex);
void initRecursiveMutex(pthread_mutex_t* mutex);
void destroyMutex(pthread_mutex_t* mutex);
void lockMutex(pthread_mutex_t* mutex);
void unlockMutex(pthread_mutex_t* mutex);

#endif
