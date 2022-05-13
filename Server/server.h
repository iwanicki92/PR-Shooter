#ifndef SERVER_H
#define SERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "server_common.h"

typedef void (*Dealocator)(unsigned char*); 

// returns 0 if no error
int runServer(Dealocator dealocator);
// stop only if runServer returned without error 
int stopServer();
void freeMessage(IncomingMessage message);
void sendTo(Message message, size_t client_id);
void sendToEveryone(Message message);
// returns first message in queue or waits for one up to wait_seconds.
// if function times out without message then returns IncomingMessage{.message_type=OTHER, Message{.size=0, .data=NULL}}
IncomingMessage takeMessage(size_t wait_seconds);

#ifdef __cplusplus
}
#endif

#endif
