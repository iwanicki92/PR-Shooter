#include "server_wrapper.hpp"
#include "../Server/server.h"

volatile sig_atomic_t Server::running = false;

IncomingMessageWrapper::IncomingMessageWrapper(IncomingMessage&& move) : incoming_message(move) {
    move.message.data = nullptr;
}

IncomingMessageWrapper::~IncomingMessageWrapper() {
    freeMessage(incoming_message);
}

unsigned char* IncomingMessageWrapper::getBuffer() {
    return incoming_message.message.data;
}

std::string IncomingMessageWrapper::toString() {
    return std::string(reinterpret_cast<char*>(getBuffer()), getSize());
}

size_t IncomingMessageWrapper::getSize() {
    return incoming_message.message.size;
}

size_t IncomingMessageWrapper::getClientId() {
    return incoming_message.client_id;
}

MessageType IncomingMessageWrapper::getType() {
    return incoming_message.message_type;
}

bool IncomingMessageWrapper::isMessage() {
    return incoming_message.message.data != nullptr;
}

bool Server::run() {
    if(isRunning() == true) {
        return false;
    }
    if(runServer([](unsigned char* ptr){ delete[] ptr;}) == 0)
        running = true;
    return isRunning();
}

void Server::stop() {
    if(isRunning() == true)
        stopServer();
}

bool Server::isRunning() {
    return running;
}

bool Server::isMessageWaiting() {
    return !isEmpty();
}

static void sendMessageTo(Message message, size_t client_id) {
    sendTo(message, client_id);
}

static void sendMessageToEveryone(Message message) {
    sendToEveryone(message);
}

IncomingMessageWrapper Server::takeMessage(size_t wait_seconds) {
    return take(wait_seconds);
}
