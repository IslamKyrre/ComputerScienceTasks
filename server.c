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


void my_handler(int sig){
    printf("CTRL-C pressed\n");
    if (unlink(ServerFIFO) < 0){
        printf("Couldn't delete FIFO");
        exit(-1);
    }
    exit(0);

}


int main() {
    int serverFd, clientFd;


    umask(0);
    if (mkfifo(ServerFIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(-1);
    }

    serverFd = OpenFD(ServerFIFO, O_RDWR);

    signal(SIGINT, my_handler);

    while (1) {

        char ClientFIFO[64];
        struct Request CurrentRequest;

        signal(SIGINT, my_handler);

        if (read(serverFd, &CurrentRequest, sizeof(struct Request)) < 0) {
            perror("Unfortunately, some of the requests are incorrect");
            continue;
        }

        sprintf(ClientFIFO, "%d.fifo", CurrentRequest.pid);

        if ((clientFd = open(ClientFIFO, O_WRONLY)) < 0) {
            perror("Unfortunately, we couldn't contact with some of the clients");
            continue;
        }


        int ClientFile;

        if ((ClientFile = open(CurrentRequest.filename, O_RDONLY)) < 0) {
            perror("Unfortunately, we couldn't open the file for client.");
            continue;
        }

        char buf[PAGE_SIZE] = "";
        int RD;

        while (1) {


            if((RD = read(ClientFile, buf, PAGE_SIZE)) < 0) {
                perror("Unfortunately, we couln't read the file.");
            }

            if(!RD) break;

            if (write(clientFd, buf, RD) < 0) {
                perror("Unfortunately, we couldn't send to the client content of the file.");
                break;
            }

        }

        close(clientFd);
        close(ClientFile);

    }
    close(serverFd);
}

