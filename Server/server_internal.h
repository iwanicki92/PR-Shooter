#ifndef SERVER_INTERNAL_H
#define SERVER_INTERNAL_H
#include "server_common.h"
#include "server_array.h"

typedef enum {RUNNING, STOP, STOPPED} ThreadStatus;

typedef struct {
    pthread_t thread_id;
    int socket;
    ThreadStatus status;
} Client;

void freeOutgoingMessage(Message message);
// returns client_id of added client. Most likely array index
void addClient(int client_socket);
Client getClient(size_t client_id);
ThreadStatus getClientStatus(size_t client_id);

#endif