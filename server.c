#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <errno.h>

#define PAGE_SIZE 4096

#define KEY_GENERATOR "server.c"

key_t shm_key, sem_key;
int shm_id, sem_id;
char *buf;
struct sembuf mybuf[4];

int main() {


    if ((shm_key = ftok(KEY_GENERATOR, 1)) < 0) {
        perror("Couldn't generate a key for shared memory");
        exit(errno);
    }

    if ((sem_key = ftok(KEY_GENERATOR, 2)) < 0) {
        perror("Couldn't generate a key for semaphore");
        exit(errno);
    }

    if ((shm_id = shmget(shm_key, PAGE_SIZE, IPC_CREAT | 0666)) < 0) {
        perror("Couldn't share a memory");
        exit(errno);
    }

    if ((buf = (char *) shmat(shm_id, NULL, 0)) == (char *) (-1)) {
        perror("Couldn't use the shared memory");
        exit(errno);
    }

    if ((sem_id = semget(sem_key, 2, IPC_CREAT | 0666)) < 0) {
        perror("Couldn't make a semaphore");
        exit(errno);
    }

    /*
     * У нас есть два семафора:
     * 0 - чтение файлы в client.c, 1 - вывод прочитанного в server.c
     * */

    while (1) {

        mybuf[0].sem_num = 0;
        mybuf[0].sem_op = 1;
        mybuf[0].sem_flg = 0;

        if (semop(sem_id, mybuf, 1) < 0) {
            perror("semop2 error\n");
            exit(errno);
        }

        mybuf[0].sem_num = 1;
        mybuf[0].sem_op = -1;
        mybuf[0].sem_flg = 0;

        mybuf[1].sem_num = 0;
        mybuf[1].sem_op = 2;
        mybuf[1].sem_flg = 0;

        mybuf[2].sem_num = 0;
        mybuf[2].sem_op = -2;
        mybuf[2].sem_flg = SEM_UNDO;

        mybuf[3].sem_num = 0;
        mybuf[3].sem_op = 0;
        mybuf[3].sem_flg = IPC_NOWAIT;

        if (semop(sem_id, mybuf, 4) < 0) {

            if (shmctl(shm_id, 0, 0) < 0) {
                perror("Couldn't free the shared memory");
                exit(errno);
            }

            if (semctl(sem_id, 0, 0) < 0) {
                perror("Couldn't delete the semaphore");
                exit(errno);
            }

            if (errno != EAGAIN) {
                perror("Seems, client.c was stopped");
            }
            exit(errno);
        }

        int size_to_write = 0;
        while (buf[size_to_write] != '#') {
            size_to_write++;
        }

        if ((write(1, buf, size_to_write)) < 0) {
            perror("Couldn't write");
            exit(errno);
        }

        mybuf[0].sem_num = 0;
        mybuf[0].sem_op = 1;
        mybuf[0].sem_flg = 0;

    }

}
