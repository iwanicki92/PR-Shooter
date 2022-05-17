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
    while(queueSyncIsEmpty(&received_messages)) {
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

IncomingMessage take(size_t wait_seconds) {
    IncomingMessage recv_msg = {.message_type = EMPTY, .message = {.size = 0, .data = NULL}};
    queueLock(&received_messages);
    if(waitForMessage(wait_seconds) == 0) {
        queueSyncPopCopyFront(&received_messages, &recv_msg);
    }
    queueUnlock(&received_messages);    
    return recv_msg;
}

bool isEmpty() {
    return queueSyncIsEmpty(&received_messages);
}

void initReceivedQueue() {
    received_messages = queueSyncCreate(sizeof(IncomingMessage));
}

void destroyReceivedQueue() {
    queueSyncDestroy(&received_messages);
}

void* startReceiving(void* client_id_arg) {
    size_t client_id = *((size_t*)client_id_arg);
    IncomingMessage connected = {.message_type = NEW_CONNECTION, .client_id = client_id, .message={.size = 0, .data = NULL}};
    queueSyncPushBack(&received_messages, &connected);
    char buf[256];
    snprintf(buf, 256, "startReceiving() client: %ld", client_id);
    printThreadDebugInformation(buf);
    free(client_id_arg);
    int sock = getClient(client_id).socket;
    struct pollfd file_descriptor = {.fd = sock, .events = POLLIN};
    while(getClientStatus(client_id) == RUNNING) {
        int return_poll = poll(&file_descriptor, 1, 2000);
        if(return_poll < 0 && errno != EINTR) {
            perror("poll() error");
            break;
        }
        else if(return_poll == 1) {
            if(file_descriptor.revents & POLLIN) {
                IncomingMessage recv_msg = {.message_type = MESSAGE, .client_id = client_id};
                int return_recv = recv(sock, &recv_msg.message.size, sizeof(recv_msg.message.size), MSG_WAITALL);
                if(return_recv == -1) {
                    if(errno != ECONNRESET && errno != EPIPE) {
                        snprintf(buf, 256, "client(%ld): recv(size) error!", client_id);
                        perror(buf);
                    }
                    break;
                }
                else if(return_recv == 0) {
                    break;
                }
                else if((size_t)return_recv != sizeof(recv_msg.message.size)) {
                    fprintf(stderr, "client(%ld): recv(size) returned wrong number of bytes: %d instead of %ld\n", client_id, return_recv, sizeof(recv_msg.message.size));
                    break;
                }
                recv_msg.message.data = malloc(recv_msg.message.size);
                return_recv = recv(sock, recv_msg.message.data, recv_msg.message.size, MSG_WAITALL);
                if(return_recv == -1) {
                    if(errno != ECONNRESET && errno != EPIPE) {
                        snprintf(buf, 50, "client(%ld): recv(data) data error!", client_id);
                        perror(buf);
                    }
                    free(recv_msg.message.data);
                    break;
                }
                else if((size_t)return_recv != recv_msg.message.size) {
                    if(return_recv != 0) {
                        fprintf(stderr, "client(%ld): recv(data) returned wrong number of bytes: %d instead of %d\n", client_id, return_recv, recv_msg.message.size);
                    }
                    free(recv_msg.message.data);
                    break;
                }
                queueLock(&received_messages);
                queueSyncPushBack(&received_messages, &recv_msg);
                pthread_cond_signal(&recv_message_cond);
                queueUnlock(&received_messages);
            }
            else {
                printf("client(%ld): File descriptor revents unknown: %d", client_id, file_descriptor.revents);
            }
        }
    }
    IncomingMessage disconnected = {.message_type = LOST_CONNECTION, .client_id = client_id, .message={.size = 0, .data = NULL}};
    queueSyncPushFront(&received_messages, &disconnected);
    sprintf(buf, "client(%ld): closing \n", client_id);
    printThreadDebugInformation(buf);
    if(close(sock) == -1) {
        perror("close() error!");
    }
    stopClient(client_id);
    return NULL;
}
