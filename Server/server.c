#include "server.h"
#include "server_internal.h"
#include "server_listen.h"
#include "server_send.h"
#include "server_receive.h"
#include "server_mutex.h"
#include "server_array.h"
#include "server_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    pthread_t listening_thread;
    pthread_t sending_thread;
    // Array of Client types
    SynchronizedArray clients;
} Threads;

static Threads threads;
static Dealocator dealocator;
static bool stopped = true;
static pthread_mutex_t stop_mutex;

static int startListeningThread();
static int startSendingThread();
static void stopReceivingThreads();
static void clearActiveClients();
static void clearConnectedClients();
static void clearEverything();

void printThreadDebugInformation(const char* msg) {
    #ifdef PRINT_DEBUG
    printf("Thread [%ld]: %s\n", pthread_self(), msg);
    #endif
}

int runServer(Dealocator dealocator_function) {
    printThreadDebugInformation("runServer()");
    srand(time(NULL));
    dealocator = dealocator_function;
    stopped = false;
    initMutex(&stop_mutex);
    threads.clients = arraySyncCreate(sizeof(Client), 16);
    // received queue needs to be initialized before first receiving thread is created
    // and destroyed after all receiving threads are joined
    initReceivedQueue();
    if(startListeningThread() != 0) {
        stopped = true;
        clearEverything();
        return 1;
    }
    if(startSendingThread() != 0) {
        lockMutex(&stop_mutex);
        stopped = true;
        unlockMutex(&stop_mutex);
        pthread_join(threads.listening_thread, NULL);
        stopReceivingThreads();
        clearEverything();
        return 2;
    }
    return 0;
}

int stopServer() {
    stopped = true;
    pthread_join(threads.listening_thread, NULL);
    pthread_join(threads.sending_thread, NULL);
    stopReceivingThreads();
    clearEverything();
    return 0;
}

bool isStopped() {
    lockMutex(&stop_mutex);
    bool stop_ret = stopped;
    unlockMutex(&stop_mutex);
    return stop_ret;
}

Client getClient(size_t client_id) {
    Client client;
    arraySyncGetItem(&threads.clients, client_id, &client);
    return client;
}

Array getAllClients() {
    arrayLock(&threads.clients);
    Array copy = arrayGetCopy(&threads.clients.array);
    arrayUnlock(&threads.clients);
    return copy;
}

ThreadStatus getClientStatus(size_t client_id) {
    arrayLock(&threads.clients);
    ThreadStatus status = ((Client*)arraySyncUnsafeGetItem(&threads.clients, client_id))->status;
    arrayUnlock(&threads.clients);
    return status;
}

void freeMessage(IncomingMessage incoming_message) {
    free(incoming_message.message.data);
}

void freeOutgoingMessage(Message message) {
    dealocator(message.data);
}

void addClient(int client_socket) {
    Client client = {.socket = client_socket, .status = RUNNING};
    arrayLock(&threads.clients);
    size_t* client_id = malloc(sizeof(size_t));
    *client_id = arrayPushBack(&threads.clients.array, &client);
    pthread_t thread_id;
    if((errno = pthread_create(&thread_id, NULL, startReceiving, client_id)) != 0) {
        perror("pthread_create() error");
        free(client_id);
        close(client_socket);
    }
    arrayUnlock(&threads.clients);
}

static int startListeningThread() {
    int err = pthread_create(&threads.listening_thread, NULL, startListening, NULL);
    if(err != 0) {
        errno = err;
        perror("Couldn't create thread listening for incoming connections");
    }
    return err;
}

static int startSendingThread() {
    int err = pthread_create(&threads.sending_thread, NULL, startSending, NULL);
    if(err != 0) {
        errno = err;
        perror("Couldn't create thread sending outgoing messages");
    }
    return err;
}

static void stopReceivingThreads() {
    arrayLock(&threads.clients);
    Array* client_array = &threads.clients.array;
    size_t size = arraySize(client_array);
    pthread_t* thread_ids = malloc(sizeof(pthread_t) * size);
    // set status to STOP for every client
    for(size_t i = 0; i < size; ++i) {
        Client* client = arrayUnsafeGetItem(client_array, i);
        client->status = STOP;
        thread_ids[i] = client->thread_id;
    }
    arrayUnlock(&threads.clients);
    // wait until all threads exit/join
    for(size_t i = 0; i < size; ++i) {
        pthread_join(thread_ids[i], NULL);
    }
    free(thread_ids);
}

static void clearActiveClients() {

}

static void clearConnectedClients() {
    arraySyncDestroy(&threads.clients);
}

static void clearEverything() {
    destroyReceivedQueue();
    clearActiveClients();
    clearConnectedClients();
    destroyMutex(&stop_mutex);
}