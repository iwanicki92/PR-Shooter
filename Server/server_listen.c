#include "server_listen.h"
#include "server_internal.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>

// copied from socket-server.c on enauczanie
void* startListening(void* arg) {
    int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("Server listening on port: %d/5000\n", serv_addr.sin_port);
    listen(listenfd, 10);
    struct pollfd file_descriptor = {.fd = listenfd, .events = POLLIN};

	while(isStopped() == false) {
        int return_poll = poll(&file_descriptor, 1, 2000);
        if(return_poll < 0 && errno != EINTR) {
            perror("poll() error");
            break;
        }
        else if(return_poll == 1) {
            if(file_descriptor.revents & POLLIN) {
                connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
                if(connfd < 0) {
                    perror("accept() error");
                } else {
                    addClient(connfd);
                }
            }
            else {
                printf("File descriptor revents unknown: %d", file_descriptor.revents);
            }
        }
	}
    close(listenfd);
    return NULL;
}