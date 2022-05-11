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
#include <errno.h>

static volatile std::sig_atomic_t stop = false;

int connectToServer();
void send(int sock, const std::string& send_string) {
    uint32_t size = static_cast<uint32_t>(send_string.length());
    send(sock, &size, 4, 0);
    send(sock, send_string.c_str(), send_string.length(), 0);
}

std::string receive(int sock);

int main() {
    // CTRL+C
    std::signal(SIGINT, [](int) { stop = true; });

    int sock = connectToServer();
    if(sock == -1) {
        return 1;
    }

    std::array<std::string, 4> test_strings{"Client1", "123456789Client123456789", "420", "@"};
    auto rng = std::bind(std::uniform_int_distribution<size_t>(0, test_strings.size() - 1), std::default_random_engine(std::random_device()()));
    int number_received = 0;
    while(stop == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        send(sock, test_strings[rng()]);
        std::string received = receive(sock);
        if(received != "") {
            ++number_received;
            std::cout << "Client: Received = " << received << "\n";
        }
    }

    if(close(sock) == -1) {
        perror("close() error!");
    }
    std::cout << "disconnected\nReceived = " << number_received << std::endl;
}

int connectToServer() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        perror("socket() error!");
        return -1;
    }

    std::string ip;
    uint16_t port;
    std::cout << "Podaj ip(albo localhost jak na tym samym komputerze): ";
    std::cin >> ip;
    std::cout << "Podaj port: ";
    std::cin >> port;

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
        perror("connect() error!");
        close(sock);
        return -1;
    }
    std::cout << "Client: connected\n";

    return sock;
}

std::string receive(int sock) {
    uint32_t size = 0;
    int return_recv = recv(sock, &size, 4, MSG_DONTWAIT);
    if(return_recv == -1) {
        if(errno != EWOULDBLOCK && errno != EAGAIN)
            perror("recv() error!");
        return "";
    }
    else if(return_recv == 0) {
        return "";
    }
    else if(return_recv != 4) {
        fprintf(stderr, "recv(size) returned wrong number of bytes: %d instead of 4\n", return_recv);
        return "";
    }
    char* buf = new char[size];
    return_recv = recv(sock, buf, size, MSG_WAITALL);
    if(return_recv == -1) {
        perror("recv() error!");
        delete[] buf;
        return "";
    }
    else if(return_recv != size) {
        fprintf(stderr, "recv(data) returned wrong number of bytes: %d instead of %d\n", return_recv, size);
        delete[] buf;
        return "";
    }
    std::string received(buf, buf + size);
    delete[] buf;
    return received;
}