#include "server_receive.h"
#include "server_queue.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

SynchronizedQueue received_queue;

IncomingMessage takeMessage() {
    IncomingMessage inc_msg;
    queueLock(&received_queue);
    // TODO #5 Do conditional wait(until message is available)
    if(queueIsEmpty(&received_queue) == false) {
        queueSyncPopCopyFront(&received_queue, &inc_msg);
    } else {
        size_t size = (size_t)(rand() % 80 + 20);
        Message msg = {.size = size, .data = malloc(size)};
        for(size_t i = 0; i < size; ++i) {
            msg.data[i] = (char)(rand() % 60 + 40);
        }
        inc_msg.message_type = MESSAGE;
        inc_msg.client_id = 0;
        inc_msg.message = msg;
    }
    queueUnlock(&received_queue);    
    
    return inc_msg;
}

void initReceivedQueue() {
    received_queue = queueSyncCreate(sizeof(IncomingMessage));
}

void destroyReceivedQueue() {
    queueSyncDestroy(&received_queue);
}

void* startReceiving(void* client_id_arg) {
    size_t client_id = *((size_t*)client_id_arg);
    char buf[50];
    snprintf(buf, 50, "startReceiving() client: %ld", client_id);
    printThreadDebugInformation(buf);
    free(client_id_arg);
    int socket = getClient(client_id).socket;

    while(getClientStatus(client_id) == RUNNING) {
        // TODO #4 Write reading from socket
    }

    close(socket);
    return NULL;
}