#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    unsigned char* data;
} Message;

typedef struct {
    enum MessageType {
        NEW_CONNECTION,
        LOST_CONNECTION,
        OTHER,
        MESSAGE
    } message_type;
    size_t client_id;
    Message message;
} IncomingMessage;

// server
bool isStopped();
// received messages buffer
bool isEmpty();

#endif
