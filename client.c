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


    int serverFd = OpenFD(ServerFIFO, O_WRONLY);


    write(serverFd, &CurrentRequest, sizeof(struct Request));

    int clientFd = OpenFD(ClientFIFO, O_RDONLY);


    char buf[PAGE_SIZE] = "";
    int RD = PAGE_SIZE;


    while (1) {

        if ((RD = read(clientFd, buf, PAGE_SIZE)) < 0) {
            perror("Client couldn't read from FIFO");
            exit(-1);
        };
        if (!RD) break;
        if ((write(1, buf, RD)) < 0) {
            perror("Client couldn't write correctly");
            exit(-1);
        };

    }

    CloseFD(clientFd);
    CloseFD(serverFd);

    if (unlink(ClientFIFO) < 0){
        printf("Couldn't delete FIFO");
        exit(-1);
    }

}
