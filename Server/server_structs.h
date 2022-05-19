#ifndef SERVER_STRUCTS_H
#define SERVER_STRUCTS_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t size;
    unsigned char* data;
} Message;

typedef struct {
    enum MessageType {
        NEW_CONNECTION,
        LOST_CONNECTION,
        EMPTY,
        MESSAGE
    } message_type;
    size_t client_id;
    Message message;
} IncomingMessage;

#endif
