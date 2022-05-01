#include "../Server/server.h"
#include <iostream>
#include <random>
#include <string>
#include <array>
#include <functional>
#include <chrono>
#include <thread>
#include <csignal>

static volatile std::sig_atomic_t stop = false;

struct IncomingMessageWrapper {
public:
    IncomingMessageWrapper() = delete;
    IncomingMessageWrapper(const IncomingMessageWrapper& copy) = delete;
    IncomingMessageWrapper(IncomingMessage&& move) : message(move) {
        move.message.data = nullptr;
    }
    ~IncomingMessageWrapper() {
        if(message.message.data != nullptr) {
            freeMessage(message);
        }
    }
    unsigned char* getBuffer() {
        return message.message.data;
    }
    std::string toString() {
        return std::string(reinterpret_cast<char*>(getBuffer()), getSize());
    }
    size_t getSize() {
        return message.message.size;
    }
private:
    IncomingMessage message;
};

int main() {
    // CTRL+C
    std::signal(SIGINT, [](int) { stop = true; });

    if(runServer([](unsigned char* ptr){ delete[] ptr;}) != 0) {
        std::cout << "Error during runServer()\n";
        return 0;
    }

    std::array<std::string, 4> test_strings{"Test string1", "Second string to send to someone", "another one(third)",
                            "Last one - fourth"};

    auto rng = std::bind(std::uniform_int_distribution<size_t>(0, test_strings.size() - 1), std::default_random_engine(std::random_device()()));

    size_t number_sent = 0;
    size_t number_received = 0;

    while(stop == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        size_t index = rng();
        const std::string& string = test_strings[index];
        unsigned char* buf = new unsigned char[string.length()];
        string.copy(reinterpret_cast<char*>(buf), string.length());
        delete[] buf;
        sendToEveryone(Message{string.length(), buf});
        ++number_sent;
        
        if(isEmpty() == false) {
            IncomingMessageWrapper message = takeMessage();
            std::cout << message.toString() << std::endl;
            ++number_received;
        }
    }

    stopServer();
    return 0;
}