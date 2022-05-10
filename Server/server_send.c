#include "server_send.h"
#include "server_internal.h"
#include "server_mutex.h"
#include "server_queue.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
    Message message;
    size_t client_id;
} IndividualMessage;

// Queue of IndividualMessages
static SynchronizedQueue outgoing_queue;
// message_to_everyone.data == NULL if there is no message
Message message_to_everyone = {.size = 0, .data = NULL};
static pthread_mutex_t message_mutex;
static pthread_cond_t new_message_cond = PTHREAD_COND_INITIALIZER;

void sendToEveryone(Message message) {
    lockMutex(&message_mutex);
    freeOutgoingMessage(message_to_everyone);
    message_to_everyone = message;
    pthread_cond_signal(&new_message_cond);
    unlockMutex(&message_mutex);
}

void sendTo(Message message, size_t client_id) {
    IndividualMessage msg = { .client_id = client_id, .message = message };
    queueSyncPushBack(&outgoing_queue, &msg);
    pthread_cond_signal(&new_message_cond);
}

// returns copy of message_to_everyone and changes message_to_everyone.data to NULL
Message popMessageToEveryone() {
    lockMutex(&message_mutex);
    Message copy = message_to_everyone;
    if(message_to_everyone.data != NULL) {
        message_to_everyone.data = NULL;
    }
    unlockMutex(&message_mutex);
    return copy;
}

void sendMessageTo(Message msg, Client client) {
    // TODO #2 write sending msg to client socket

    send(client.socket, msg.data, msg.size, 0);
    freeOutgoingMessage(msg);
}

void* startSending(void* no_arg) {
    printThreadDebugInformation("startSending()");
    initRecursiveMutex(&message_mutex);
    outgoing_queue = queueSyncCreate(sizeof(IndividualMessage));
    struct timespec time;
    while(isStopped() == false) {
        lockMutex(&message_mutex);
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        while(queueIsEmpty(&outgoing_queue) && message_to_everyone.data == NULL) {
            // wait for pthread_cond_signal or timeout
            int err = pthread_cond_timedwait(&new_message_cond, &message_mutex, &time);
            if(err == ETIMEDOUT) {
                continue;
            } else if(err < 0) {
                errno = err;
                perror("pthread_cond_timedwait() error");
                exit(1);
            }
        }
        queueLock(&outgoing_queue);        
        if(queueIsEmpty(&outgoing_queue) == false) {
            IndividualMessage* msg = queueSyncPopFront(&outgoing_queue);
            queueUnlock(&outgoing_queue);
            sendMessageTo(msg->message, getClient(msg->client_id));
        } else {
            queueUnlock(&outgoing_queue);
            Message msg = popMessageToEveryone();
            unlockMutex(&message_mutex);
            Array clients = getAllClients();
            for(size_t i = 0; i < clients.size; ++i) {
                Client client = *(Client*)(arrayUnsafeGetItem(&clients, i));
                sendMessageTo(msg, client);
            }
            arrayDestroy(&clients);
            continue; // because I already unlocked message_mutex
        }
        unlockMutex(&message_mutex);
    }

    queueSyncDestroy(&outgoing_queue);
    destroyMutex(&message_mutex);
    freeOutgoingMessage(message_to_everyone);
    return NULL;
}