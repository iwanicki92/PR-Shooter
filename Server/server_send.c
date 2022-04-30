#include "server_send.h"
#include "server_internal.h"
#include "server_mutex.h"
#include <unistd.h>

// to_everyone.data == NULL if there is no message
Message to_everyone = {.size = 0, .data = NULL};
static pthread_mutex_t mutex;


void clearOutgoingMessages() {

}

void sendToEveryone(Message message) {
    lockMutex(&mutex);
    freeOutgoingMessage(to_everyone);
    to_everyone = message;
    unlockMutex(&mutex);
}

// returns copy of to_everyone and changes to_everyone.data to NULL
Message popMessageToEveryone() {
    lockMutex(&mutex);
    Message copy = to_everyone;
    if(to_everyone.data != NULL) {
        to_everyone.data = NULL;
    }
    unlockMutex(&mutex);
    return copy;
}

void* startSending(void* no_arg) {
    initMutex(&mutex);
    while(isStopped() == false) {
        Message broadcast = popMessageToEveryone();
        sleep(1);
        freeOutgoingMessage(broadcast);
    }
    return NULL;
}