#include "server_receive.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void* startReceiving(void* client_id_arg) {
    size_t client_id = *((size_t*)client_id_arg);
    free(client_id_arg);
    int socket = getClient(client_id).socket;

    while(getClientStatus(client_id) == RUNNING) {
        printf("RECEIVING\n");
        sleep(1);
    }

    close(socket);
    return NULL;
}