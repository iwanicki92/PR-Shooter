#include "server_receive.h"
#include "server_internal.h"
#include "server_queue.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>

static SynchronizedQueue received_messages;
static pthread_cond_t recv_message_cond = PTHREAD_COND_INITIALIZER;

static int waitForMessage(size_t seconds) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += (time_t)seconds;
    while(queueIsEmpty(&received_messages)) {
        // wait for pthread_cond_signal or timeout
        int err = pthread_cond_timedwait(&recv_message_cond, received_messages.mutex, &time);
        if(err == ETIMEDOUT) {
            return ETIMEDOUT;
        } else if(err != 0) {
            errno = err;
            perror("pthread_cond_timedwait() error");
            exit(1);
        }
    }
    return 0;
}

IncomingMessage takeMessage(size_t wait_seconds) {
    IncomingMessage recv_msg = {.message_type = OTHER, .message = {.size = 0, .data = NULL}};
    queueLock(&received_messages);
    if(waitForMessage(wait_seconds) == 0) {
        queueSyncPopCopyFront(&received_messages, &recv_msg);
    }
    queueUnlock(&received_messages);    
    return recv_msg;
}

bool isEmpty() {
    //return queueIsEmpty(&received_messages);
    return false; // for debugging, delete later
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
                queueLock(&received_messages);
                queueSyncPushBack(&received_messages, &recv_msg);
                pthread_cond_signal(&recv_message_cond);
                queueUnlock(&received_messages);
            }
            else {
                printf("File descriptor revents unknown: %d", file_descriptor.revents);
            }
        }
    }

    close(socket);
    return NULL;
}
