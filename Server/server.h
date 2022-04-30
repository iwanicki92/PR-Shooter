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
void sendToEveryone(Message message);
IncomingMessage takeMessage();

#ifdef __cplusplus
}
#endif

#endif