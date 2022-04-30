#include "server.h"
#include "server_internal.h"
#include "server_listen.h"
#include "server_send.h"
#include "server_receive.h"
#include "server_mutex.h"
#include "server_array.h"
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

Threads threads;
// TODO make SynchronizedQueue
SynchronizedArray received_messages;
static Dealocator dealocator;
static bool stopped = true;
pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;

int startListeningThread();
int startSendingThread();
void stopListeningThread();
void stopSendingThread();
void stopReceivingThreads();
void clearReceivedMessages();
void clearOutgoingMessages();
void clearActiveClients();
void clearConnectedClients();
void clearEverything();

int runServer(Dealocator dealocator_function) {
    srand(time(NULL));
    dealocator = dealocator_function;
    stopped = false;
    initMutex(&stop_mutex);
    received_messages = arraySyncCreate(sizeof(IncomingMessage), 16);
    threads.clients = arraySyncCreate(sizeof(Client), 16);
    if(startListeningThread() != 0) {
        clearEverything();
        return 1;
    }
    if(startSendingThread() != 0) {
        stopListeningThread();
        stopReceivingThreads();
        clearEverything();
        return 2;
    }
    return 0;
}

int stopServer() {
    stopListeningThread();
    stopSendingThread();
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

bool isEmpty() {
    return false;
}

IncomingMessage takeMessage() {
    size_t size = (size_t)(rand() % 80 + 20);
    Message msg = {.size = size, .data = malloc(size)};
    IncomingMessage inc_msg = {.message_type = MESSAGE, .client_id = 0, .message = msg };
    return inc_msg;
}

int startListeningThread() {
    int err = pthread_create(&threads.listening_thread, NULL, startListening, NULL);
    if(err != 0) {
        errno = err;
        perror("Couldn't create thread listening for incoming connections");
    }
    return err;
}

int startSendingThread() {
    int err = pthread_create(&threads.sending_thread, NULL, startSending, NULL);
    if(err != 0) {
        errno = err;
        perror("Couldn't create thread sending outgoing messages");
    }
    return err;
}

void stopListeningThread() {
    lockMutex(&stop_mutex);
    stopped = true;
    unlockMutex(&stop_mutex);
    pthread_join(threads.listening_thread, NULL);
}

void stopSendingThread() {
    
}

void stopReceivingThreads() {
    arrayLock(&threads.clients);
    Array* client_array = &threads.clients.array;
    size_t size = arraySize(client_array);
    pthread_t* thread_ids = malloc(sizeof(pthread_t) * size);
    for(size_t i = 0; i < size; ++i) {
        Client* client = arrayUnsafeGetItem(client_array, i);
        client->status = STOP;
        thread_ids[i] = client->thread_id;
    }
    arrayUnlock(&threads.clients);
    for(size_t i = 0; i < size; ++i) {
        pthread_join(thread_ids[i], NULL);
    }
    free(thread_ids);
}

void clearReceivedMessages() {}
void clearOutgoingMessages() {}
void clearActiveClients() {}
void clearConnectedClients() {
    arraySyncDestroy(&threads.clients);
}

void clearEverything() {
    clearReceivedMessages();
    clearOutgoingMessages();
    clearActiveClients();
    clearConnectedClients();
    destroyMutex(&stop_mutex);
}