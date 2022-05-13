#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <iostream>
#include <array>
#include <string>
#include <random>
#include <functional>
#include <chrono>
#include <thread>
#include <cerrno>

static volatile std::sig_atomic_t stop = false;

int connectToServer(const std::string& ip, uint16_t port = 5000);
void send(int sock, const std::string& send_string, int& sent);
std::string receive(int sock);

std::string getIP() {
    std::string ip;
    std::cout << "Podaj ip(albo localhost jak na tym samym komputerze): ";
    std::cin >> ip;
    return ip;
}

uint16_t getPort() {
    uint16_t port;
    std::cout << "Podaj port(domyślny 5000): ";
    std::cin >> port;
    return port;
}

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

int main(int argc, char* argv[]) {
    // CTRL+C
    std::signal(SIGINT, [](int) { stop = true; });

    std::string ip = argc > 1 ? argv[1] : getIP();
    uint16_t port = argc > 2 ? static_cast<uint16_t>(std::stoi(argv[2])) : getPort();

    int sock = connectToServer(ip, port);
    if(sock == -1) {
        return 1;
    }

    std::array<std::string, 4> test_strings{"Client1", "123456789Client123456789", "420", "@"};
    auto rng = std::bind(std::uniform_int_distribution<size_t>(0, test_strings.size() - 1), std::default_random_engine(std::random_device()()));
    int number_received = 0;
    int number_sent = 0;
    Timer timer;
    while(stop == false) {
        if(timer.time() > 0.01) {
            send(sock, test_strings[rng()], number_sent);
            timer.start();
        }
        std::string received = receive(sock);
        if(received != "") {
            ++number_received;
        }
    }

    if(close(sock) == -1) {
        perror("Client: close() error!");
    }
    printf("Client: Disconnected\nClient: Received %d messages, Sent %d messages\n", number_received, number_sent);
    fflush(0);
    return 0;
}

void send(int sock, const std::string& send_string, int& sent) {
    uint32_t size = static_cast<uint32_t>(send_string.length());
    if(send(sock, &size, 4, 0) == -1) {
        if(errno != ECONNRESET && errno != EPIPE) {
            perror("Client: send(size) error");
        }
        return;
    }
    if(send(sock, send_string.c_str(), send_string.length(), 0) == -1) {
        if(errno != ECONNRESET && errno != EPIPE) {
            perror("Client: send(data) error");
        }
        return;
    }
    ++sent;
}

int connectToServer(const std::string& ip, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        perror("socket() error!");
        return -1;
    }

    struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(ip != "localhost") {
        inet_aton(ip.c_str(), &serv_addr.sin_addr);
    } else {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if(connect(sock, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) == -1) {
        perror("Client: connect() error!");
        close(sock);
        return -1;
    }
    std::cout << "Client: Connected\n";

    return sock;
}

std::string receive(int sock) {
    uint32_t size = 0;
    int return_recv = recv(sock, &size, 4, MSG_DONTWAIT);
    if(return_recv == -1) {
        if(errno != EWOULDBLOCK && errno != EAGAIN && errno != ECONNRESET && errno != EPIPE) {
            perror("Client: recv() error!");
        }
        return "";
    }
    else if(return_recv == 0) {
        return "";
    }
    else if(return_recv != 4) {
        fprintf(stderr, "Client: recv(size) returned wrong number of bytes: %d instead of 4\n", return_recv);
        return "";
    }
    char* buf = new char[size];
    return_recv = recv(sock, buf, size, MSG_WAITALL);
    if(return_recv == -1) {
        if(errno != ECONNRESET && errno != EPIPE) {
            perror("Client: recv() error!");
        }
        delete[] buf;
        return "";
    }
    else if(return_recv != size) {
        fprintf(stderr, "Client: recv(data) returned wrong number of bytes: %d instead of %d\n", return_recv, size);
        delete[] buf;
        return "";
    }
    std::string received(buf, buf + size);
    delete[] buf;
    return received;
}