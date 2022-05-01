#include "server_receive.h"
#include "server_queue.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

SynchronizedQueue received_queue;

IncomingMessage takeMessage() {
    IncomingMessage msg;
    queueLock(&received_queue);
    // TODO #5 Do conditional wait(until message is available)
    if(queueIsEmpty(&received_queue)) {
        queueSyncPopCopyFront(&received_queue, &msg);
    } else {
        size_t size = (size_t)(rand() % 80 + 20);
        Message msg = {.size = size, .data = malloc(size)};
        IncomingMessage inc_msg = {.message_type = MESSAGE, .client_id = 0, .message = msg };
    }
    queueUnlock(&received_queue);    
    
    return msg;
}

void* startReceiving(void* client_id_arg) {
    queueSyncCreate(sizeof(IncomingMessage));
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