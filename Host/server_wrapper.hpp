#pragma once
#include "../Server/server_structs.h"
#include <string>
#include <csignal>

using MessageType = IncomingMessage::MessageType;

struct IncomingMessageWrapper {
public:
    IncomingMessageWrapper() = delete;
    IncomingMessageWrapper(const IncomingMessageWrapper& copy) = delete;
    IncomingMessageWrapper(IncomingMessage&& move);
    ~IncomingMessageWrapper();

    unsigned char* getBuffer();
    std::string toString();
    size_t getSize();
    size_t getClientId();
    MessageType getType();
    bool isMessage();
private:
    IncomingMessage incoming_message;
};

class Server {
public:
    // true if everything ok
    static bool run();
    static void stop();
    static bool isRunning();
    static bool isMessageWaiting();
    static void sendMessageTo(Message message, size_t client_id);
    static void sendMessageToEveryone(Message message);
    static IncomingMessageWrapper takeMessage(size_t wait_seconds = 0);
private:
    volatile static std::sig_atomic_t running;
};
