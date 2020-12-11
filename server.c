#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define ServerFIFO "file.fifo"

#define PAGE_SIZE 4096

struct Request {
    pid_t pid;
    char filename[256];

};

void CloseFD(int fd) {
    if (close(fd) == -1) {
        perror("Server couldn't close the file");
        exit(-1);
    }
}

int OpenFD(const char *name, int flags) {

    int fd;
    if ((fd = open(name, flags)) < 0) {
        printf("Server couldn't open %s\n", name);
        exit(-1);
    }
    return fd;
}

void TurnOffFlag_NONBLOCK(int fd) {
    int flags = (fcntl(fd, F_GETFL)) & ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}


int main() {
    int serverFd, clientFd;


    umask(0);
    if (mkfifo(ServerFIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(-1);
    }

    serverFd = OpenFD(ServerFIFO, O_RDWR);


    while (1) {

        char ClientFIFO[64];
        struct Request CurrentRequest;

        if (read(serverFd, &CurrentRequest, sizeof(struct Request)) < 0) {
            perror("Unfortunately, some of the requests are incorrect");
            continue;
        }

        sprintf(ClientFIFO, "%d.fifo", CurrentRequest.pid);

        if ((clientFd = open(ClientFIFO, O_WRONLY | O_NONBLOCK)) < 0) {
            perror("Unfortunately, we couldn't contact with some of the clients");
            continue;
        }
        TurnOffFlag_NONBLOCK(clientFd);

        FILE *ClientFile;

        if ((ClientFile = fopen(CurrentRequest.filename, "r")) == NULL) {
            perror("Unfortunately, we couldn't open the file for client.");
            continue;
        }

        char buf[PAGE_SIZE] = "";
        int RD;


        while ((RD = fread(buf, sizeof(char), PAGE_SIZE, ClientFile)) == PAGE_SIZE) {

            if (write(clientFd, buf, PAGE_SIZE) < 0) {
                perror("Unfortunately, we couldn't send to the client content of the file.");
                break;
            }

        }

        write(clientFd, buf, RD);


        close(clientFd);
    }
}

#pragma clang diagnostic pop