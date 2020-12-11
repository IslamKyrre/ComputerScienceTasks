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

#define ServerFIFO "file.fifo"

#define PAGE_SIZE 4096


struct Request {
    pid_t pid;
    char filename[256];

};

void CloseFD(int fd) {
    if (close(fd) == -1) {
        perror("Client couldn't close the file");
        exit(-1);
    }
}

int OpenFD(const char *name, int flags) {

    int fd;
    if ((fd = open(name, flags)) < 0) {
        printf("Client couldn't open %s \n", name);
        exit(-1);
    }
    return fd;
}

void TurnOffFlag_NONBLOCK(int fd) {
    int flags = (fcntl(fd, F_GETFL)) & ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        perror("Please, make sure, your arguments are correct!\n");
        exit(-1);
    }

    struct Request CurrentRequest;
    CurrentRequest.pid = getpid();
    strcpy(CurrentRequest.filename, argv[1]);


    umask(0);
    char ClientFIFO[64];
    sprintf(ClientFIFO, "%d.fifo", getpid());

    if (mkfifo(ClientFIFO, 0666) == -1 && errno != EEXIST) {
        perror("Server couldn't make FIFO");
        exit(-1);
    }


    int serverFd = OpenFD(ServerFIFO, O_WRONLY | O_NONBLOCK);
    TurnOffFlag_NONBLOCK(serverFd);


    int clientFd = OpenFD(ClientFIFO, O_RDONLY | O_NONBLOCK);
    TurnOffFlag_NONBLOCK(clientFd);

    write(serverFd, &CurrentRequest, sizeof(struct Request));

    char buf[PAGE_SIZE] = "";
    int RD = PAGE_SIZE;

    sleep(1);

    while (1) {

        RD = read(clientFd, buf, PAGE_SIZE);
        if (!RD) break;
        write(1, buf, RD);

    }

    CloseFD(clientFd);
    CloseFD(serverFd);

}