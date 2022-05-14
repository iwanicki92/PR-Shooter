#include "../Server/server.h"
#include <iostream>
#include <random>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <functional>
#include <chrono>
#include <thread>
#include <csignal>

static volatile std::sig_atomic_t stop = false;

using MessageType = IncomingMessage::MessageType;

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
    size_t getClientId() {
        return message.client_id;
    }
    MessageType getType() {
        return message.message_type;
    }
    bool isMessage() {
        return message.message.data != nullptr;
    }
private:
    IncomingMessage message;
};

struct Timer {
    void start() {
        time_priv = std::chrono::steady_clock::now();
    }
    auto time() {
        return  std::chrono::duration<double>(std::chrono::steady_clock::now() - time_priv).count();
    }
    private:
        std::chrono::time_point<std::chrono::steady_clock> time_priv = std::chrono::steady_clock::now();
};

struct Host {
    std::set<size_t> clients;
    // client_id, (sent, received)
    std::map<size_t, std::pair<size_t, size_t>> client_messages;

    void addClient(size_t client_id) {
        clients.insert(client_id);
    }

    void send(const std::string& string) {
        for(size_t client_id : clients) {
            unsigned char* buf = new unsigned char[string.length() + 1];
            string.copy(reinterpret_cast<char*>(buf), string.length());
            buf[string.length()] = '\0';
            //printf("sending [%s] to: %ld\n", buf, string.length());
            sendTo(Message{static_cast<uint32_t>(string.length()), buf}, client_id);
            client_messages[client_id].first += 1;
        }
    }

    void takeMessage() {
        if(isEmpty() == false) {
            IncomingMessageWrapper message = take(1);
            switch(message.getType()) {
                case MessageType::NEW_CONNECTION:
                    addClient(message.getClientId());
                    break;
                case MessageType::LOST_CONNECTION:
                    clients.erase(message.getClientId());
                    break;
                case MessageType::MESSAGE:
                    //printf("Host: received from client(%ld): %s\n", message.getClientId(), message.toString().c_str());
                    //fflush(stdout);
                    client_messages[message.getClientId()].second += 1;
                    break;
            }
        }
    }
};

void sigHandler(int) {
    stop = true;
}

int main() {
    struct sigaction action = {.sa_flags = SA_RESTART};
    action.sa_handler = &sigHandler;
    sigfillset(&action.sa_mask);
    // CTRL+C
    sigaction(SIGINT, &action, NULL);
    
    sigset_t set, old_set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, &old_set);

    printf("Host: starting server\n");
    if(runServer([](unsigned char* ptr){ delete[] ptr;}) != 0) {
        std::cout << "Error during runServer()\n";
        return 0;
    }
    pthread_sigmask(SIG_SETMASK, &old_set, NULL);
    fflush(0);
    Host host;
    std::array<std::string, 4> test_strings{"Test string1", "Second string to send to someone", "another one(third)",
                            "Last one - fourth"};
    auto rng = std::bind(std::uniform_int_distribution<size_t>(0, test_strings.size() - 1), std::default_random_engine(std::random_device()()));

    Timer timer;
    while(stop == false) {
        size_t index = rng();
        const std::string& string = test_strings[index];
        if(timer.time() > 0.01) {
            host.send(string);
            timer.start();
        }
        host.takeMessage();
    }

    printf("Server: stopping\n");
    fflush(0);
    stopServer();
    for(const auto& [key, value] : host.client_messages) {
        printf("Host: client(%ld) sent to: %ld, received from: %ld\n", key, value.first, value.second);
    }
    fflush(0);
    return 0;
}
