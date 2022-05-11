#ifndef SERVER_RECEIVE_H
#define SERVER_RECEIVE_H

// client_id should be dynamically allocated size_t*. startReceiving will free client_id memory
void* startReceiving(void* client_id);
void initReceivedQueue();
void destroyReceivedQueue();

#endif
