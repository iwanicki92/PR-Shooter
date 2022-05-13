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

void printThreadDebugInformation(const char* msg);
void freeOutgoingMessage(Message message);
void addClient(int client_socket);
void stopClient(size_t client_id);
void signalClientToStop(size_t client_id);
Client getClient(size_t client_id);
// returns a copy. Needs to be destroyed after use!!!
Array getAllClients();
ThreadStatus getClientStatus(size_t client_id);

#endif
