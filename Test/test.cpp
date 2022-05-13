#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <csignal>
#include <cerrno>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>

pid_t createHost(std::string path, int pipefd[2]);
pid_t createClient(std::string path);
void sendInterrupt(pid_t process_id);
void waitForInterrupt();
void printPipe(int pipefd);

int main(int argc, char *argv[]) {
    pid_t host_pid;
    std::vector<pid_t> client_pids;
    int host_pipefd[2];
    if(pipe(host_pipefd) == -1) {
        perror("pipe() error!");
    }
    int run_seconds = argc > 1 ? std::stoi(argv[1]) : 2;
    size_t number_of_clients = argc > 2 ? std::stoi(argv[2]) : 4;
    
    char buf[512];
    char* dir = dirname(argv[0]);
    if(realpath(dir, buf) == nullptr) {
        perror("realpath() error!");
        return 1;
    }
    std::string path(buf);

    host_pid = createHost(path + "/test_host", host_pipefd);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for(size_t i = 0; i < number_of_clients; ++i) {
        pid_t id = createClient(path + "/test_client");
        if(id > 0) {
            client_pids.push_back(id);
            //printf("Client process(%d) started\n", id);
        }
    }

    if(run_seconds == -1) {
        waitForInterrupt();
    }
    else {
        std::this_thread::sleep_for(std::chrono::seconds(run_seconds));
    }
    
    for(size_t i = 0; i < client_pids.size(); ++i) {
        sendInterrupt(client_pids[i]);
    }
    int w = 0;
    for(size_t i = 0; i < client_pids.size(); ++i) {
        pid_t return_id = waitpid(client_pids[i], &w, 0);
        if(return_id == -1) {
            perror("wait(client) error!");
        } else {
            //printf("Client Process(%d) completed\n", return_id);
        }
    }
    if(host_pid > 0) {
        sendInterrupt(host_pid);
    }
    printPipe(host_pipefd[0]);
    pid_t return_id = waitpid(host_pid, &w, 0);
    if(return_id == -1) {
        perror("waitpid(host) error!");
    } else {
        //printf("Host Process(%d) completed\n", return_id);
    }

    printf("Test complete\n");
    if(close(host_pipefd[0]) == -1) {
        perror("close(host) error");
    }
    return 0;
}

pid_t createHost(std::string path, int pipefd[2]) {
    pid_t id = fork();
    if(id == -1) {
        perror("fork() error");
    }
    else if(id == 0) {
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2() error!");
        }
        if(dup2(pipefd[1], STDERR_FILENO) == -1) {
            perror("dup2() error!");
        }
        if(close(pipefd[0]) == -1) {
            perror("fork close(pipefd[0]) error!");
        }
        if(close(pipefd[1]) == -1) {
            perror("fork close(pipefd[1]) error!");
        }
        char buf[512];
        path.copy(buf, path.length());
        buf[path.length()] = '\0';
        if(execl(buf, buf, NULL) == -1) {
            perror("execl() error!");
            fprintf(stderr, "path err: %s\n", buf);
            _exit(1);
        }
    }
    if(close(pipefd[1]) == -1) {
        perror("close(pipefd[1]) error!");
    }
    //printf("Created host,   pid = %d\n", id);
    return id;
}

pid_t createClient(std::string path) {
    pid_t id = fork();
    if(id == -1) {
        perror("fork() error");
    }
    else if(id == 0) {
        char buf[512];
        char arg1[] = "localhost";
        char arg2[] = "5000";
        path.copy(buf, path.length());
        buf[path.length()] = '\0';
        if(execl(buf, buf, arg1, arg2, NULL) == -1) {
            perror("execl() error!");
            fprintf(stderr, "path err: %s\n", buf);
            _exit(1);
        }
    }
    //printf("Created client, pid = %d\n", id);
    return id;
}

void sendInterrupt(pid_t process_id) {
    fflush(stdout);
    if(kill(process_id, SIGINT) == -1) {
        perror("kill() error!");
    }
}

void waitForInterrupt() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    int sig;
    int err = sigwait(&sigset, &sig);
    if(err > 0) {
        errno = err;
        perror("sigwait() error!");
    } else if(sig != SIGINT) {
        printf("sig(%d) != SIGINT(%d)", sig, SIGINT);
    }
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

void printPipe(int pipefd) {
    char buf[4096];
    FILE* pipe_read = fdopen(pipefd, "r");
    if(pipe_read == nullptr) {
        perror("fdopen(host) error!");
    }
    if (fread(buf, 1, sizeof(buf), pipe_read)) {
        printf("%s", buf);
    } else {
        printf("Host didn't write anything!\n");
  }
}