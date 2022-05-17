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
#include <signal.h>
#include <stdatomic.h>

typedef struct {
    pthread_t listening_thread;
    pthread_t sending_thread;
    // Array of Client types
    SynchronizedArray clients;
} Threads;

static Threads threads;
static Dealocator dealocator;
static volatile atomic_bool stopped = true;

static int startListeningThread();
static int startSendingThread();
static void stopReceivingThreads();
static void clearActiveClients();
static void clearConnectedClients();
static void clearEverything();

void printThreadDebugInformation(const char* msg) {
    #if (defined PRINT_DEBUG && PRINT_DEBUG >= 0)
    printf("Thread [%ld]: %s\n", pthread_self(), msg);
    #endif
}

static void setStop() {
    stopped = true;
}

int runServer(Dealocator dealocator_function) {
    sigset_t set, old_set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, &old_set);
    
    printThreadDebugInformation("runServer()");
    srand(time(NULL));
    dealocator = dealocator_function;
    stopped = false;
    threads.clients = arraySyncCreate(sizeof(Client), 16);
    // received queue needs to be initialized before first receiving thread is created
    // and destroyed after all receiving threads are joined
    initReceivedQueue();
    initOutgoingQueue();
    if(startListeningThread() != 0) {
        stopped = true;
        clearEverything();
        pthread_sigmask(SIG_SETMASK, &old_set, NULL);
        return 1;
    }
    if(startSendingThread() != 0) {
        setStop();
        pthread_join(threads.listening_thread, NULL);
        stopReceivingThreads();
        clearEverything();
        pthread_sigmask(SIG_SETMASK, &old_set, NULL);
        return 2;
    }
    pthread_sigmask(SIG_SETMASK, &old_set, NULL);
    return 0;
}

void stopServer() {
    printThreadDebugInformation("stopping server");
    setStop();
    pthread_join(threads.listening_thread, NULL);
    pthread_join(threads.sending_thread, NULL);
    stopReceivingThreads();
    clearEverything();
}

bool isStopped() {
    return stopped;
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

void stopClient(size_t client_id) {
    arrayLock(&threads.clients);
    Client* client = arraySyncUnsafeGetItem(&threads.clients, client_id);
    client->status = STOPPED;
    arrayUnlock(&threads.clients);
}

void signalClientToStop(size_t client_id) {
    arrayLock(&threads.clients);
    Client* client = arraySyncUnsafeGetItem(&threads.clients, client_id);
    client->status = STOP;
    arrayUnlock(&threads.clients);
}

void addClient(int client_socket) {
    Client client = {.socket = client_socket, .status = RUNNING};
    arrayLock(&threads.clients);
    size_t* client_id_arg = malloc(sizeof(size_t));
    *client_id_arg = arrayPushBack(&threads.clients.array, &client);
    size_t client_id = *client_id_arg;
    pthread_t thread_id;
    if((errno = pthread_create(&thread_id, NULL, startReceiving, client_id_arg)) != 0) {
        perror("pthread_create() error");
        free(client_id_arg);
        close(client_socket);
    }
    ((Client*)arraySyncUnsafeGetItem(&threads.clients, client_id))->thread_id = thread_id;
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
        int err = pthread_join(thread_ids[i], NULL);
        if(err != 0) {
            errno = err;
            perror("pthread_join() error!");
        }
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
    destroyOutgoingQueue();
    clearActiveClients();
    clearConnectedClients();
}
