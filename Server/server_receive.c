#include "server_receive.h"
#include "server_queue.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>

SynchronizedQueue received_messages;

IncomingMessage takeMessage() {
    IncomingMessage recv_msg;
    queueLock(&received_messages);
    // TODO #5 Do conditional wait(until message is available)
    if(queueIsEmpty(&received_messages) == false) {
        queueSyncPopCopyFront(&received_messages, &recv_msg);
    } else {
        size_t size = (size_t)(rand() % 80 + 20);
        Message msg = {.size = size, .data = malloc(size)};
        for(size_t i = 0; i < size; ++i) {
            msg.data[i] = (unsigned char)(rand() % 60 + 40);
        }
        recv_msg.message_type = MESSAGE;
        recv_msg.client_id = 0;
        recv_msg.message = msg;
    }
    queueUnlock(&received_messages);    
    
    return recv_msg;
}

void initReceivedQueue() {
    received_messages = queueSyncCreate(sizeof(IncomingMessage));
}

void destroyReceivedQueue() {
    queueSyncDestroy(&received_messages);
}

void* startReceiving(void* client_id_arg) {
    size_t client_id = *((size_t*)client_id_arg);
    char buf[50];
    snprintf(buf, 50, "startReceiving() client: %ld", client_id);
    printThreadDebugInformation(buf);
    free(client_id_arg);
    int socket = getClient(client_id).socket;
    struct pollfd file_descriptor = {.fd = socket, .events = POLLIN};
    while(getClientStatus(client_id) == RUNNING) {
        int return_poll = poll(&file_descriptor, 1, 2000);
        if(return_poll < 0 && errno != EINTR) {
            perror("poll() error");
            break;
        }
        else if(return_poll == 1) {
            if(file_descriptor.revents & POLLIN) {
                IncomingMessage recv_msg = {.message_type = MESSAGE, .client_id = client_id};
                int return_recv = recv(socket, &recv_msg.message.size, sizeof(size_t), MSG_WAITALL);
                if(return_recv == -1) {
                    perror("recv() msg size error!");
                    break;
                }
                else if((size_t)return_recv != sizeof(size_t)) {
                    fprintf(stderr, "recv() size returned wrong number of bytes: %d instead of %ld\n", return_recv, sizeof(size_t));
                    break;
                }
                recv_msg.message.data = malloc(recv_msg.message.size);
                return_recv = recv(socket, recv_msg.message.data, recv_msg.message.size, MSG_WAITALL);
                if(return_recv == -1) {
                    perror("recv() msg data error!");
                    free(recv_msg.message.data);
                    break;
                }
                else if((size_t)return_recv != recv_msg.message.size) {
                    fprintf(stderr, "recv() data returned wrong number of bytes: %d instead of %ld\n", return_recv, recv_msg.message.size);
                    free(recv_msg.message.data);
                    break;
                }
                queueSyncPushBack(&received_messages, &recv_msg);
            }
            else {
                printf("File descriptor revents unknown: %d", file_descriptor.revents);
            }
        }
    }

    close(socket);
    return NULL;
}