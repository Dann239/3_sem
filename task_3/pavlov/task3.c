#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>

#define min(a,b) (a < b ? a : b)
#define BUFF_SIZE (1 << 12)

char* input_name = "data.mp4";

char* data = NULL;
int memsize = 0;

void resize(int _size) {
    if(!data) {
        data = malloc(1);
        memsize = 1;
    }
    while(_size > memsize) {
        char* buff = data;
        data = malloc(memsize * 2);
        memcpy(data, buff, memsize);
        free(buff);
        memsize *= 2;
    }
}

long long get_time() {
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * 1000000 + currentTime.tv_usec;
}

typedef struct {
    long mtype;
    char mtext[BUFF_SIZE + 1];
} msgbuf;

msgbuf buff;

int main() {
    system("rm -f output*");
    int input = open(input_name, O_RDONLY);
    if(input == -1) {
        printf("Can't open the input file\n");
        return 0;
    }
    int size = 0, delta;
    do {
        resize(size + BUFF_SIZE);
        delta = read(input, &data[size], BUFF_SIZE);
        if(delta == -1) {
            printf("Error reading the input file\n");
            return 0;
        }
        size += delta;
    } while(delta > 0);

    srand(get_time());
    int key1 = rand(), key2 = rand();
    int cid = fork();

    int fifo_dt = 0, msg_dt = 0, shm_dt = 0;
    int t0 = get_time();
    mknod("/tmp/task3fifo", S_IFIFO | 0644, 0);
    if(cid == 0) {
        int pipe = open("/tmp/task3fifo", O_WRONLY);
        for(int i = 0; i < size; i += BUFF_SIZE)
            write(pipe, &data[i], min(size - i, BUFF_SIZE));
        close(pipe);
    }
    else {
        memset(data, 0, memsize);
        int pipe = open("/tmp/task3fifo", O_RDONLY);
        size = 0;
        do {
            delta = read(pipe, &data[size], BUFF_SIZE);
            size += delta;
        } while(delta > 0);

        int output = open("output_fifo.mp4", O_WRONLY | O_CREAT, 0644);
        for(int i = 0; i < size; i += BUFF_SIZE)
            write(output, &data[i], min(size - i, BUFF_SIZE));
        close(pipe);

        fifo_dt = get_time() - t0;
        printf("fifo: %dus\n", fifo_dt);
    }

    if(BUFF_SIZE <= 8192) {
        t0 = get_time();
        if(cid == 0) {
            int msgqid = msgget(key1, IPC_CREAT | 0644);

            if(msgqid == -1)
                printf("%s\n", strerror(errno));

            buff.mtype = 5;
            for(int i = 0; i < size; i += BUFF_SIZE) {
                memcpy(buff.mtext, &data[i], min(size - i, BUFF_SIZE));
                msgsnd(msgqid, &buff, BUFF_SIZE, 0);
            }
        }
        else {
            memset(data, 0, memsize);

            int msgqid = msgget(key1, IPC_CREAT | 0644);

            if(msgqid == -1)
                printf("%s\n", strerror(errno));

            for(int i = 0; i < size; i += BUFF_SIZE) {
                msgrcv(msgqid, &buff, BUFF_SIZE + 1, 5, 0);
                memcpy(&data[i], buff.mtext, min(size - i, BUFF_SIZE));
            }

            int output = open("output_msg.mp4", O_WRONLY | O_CREAT, 0644);
            for(int i = 0; i < size; i += BUFF_SIZE)
                write(output, &data[i], min(size - i, BUFF_SIZE));

            msg_dt = get_time() - t0;
            printf("msg: %dus\n", msg_dt);
        }
    }

    t0 = get_time();
    if(cid == 0) {
        char* data_ptr;
        int shmid = shmget(key2, BUFF_SIZE + 1, IPC_CREAT | 0666);
        data_ptr = shmat(shmid, NULL, 0);
        data_ptr[BUFF_SIZE] = 0;

        for(int i = 0; i < size; i += BUFF_SIZE) {
            memcpy(data_ptr, &data[i], min(size - i, BUFF_SIZE));
            data_ptr[BUFF_SIZE] = 42;
            while(data_ptr[BUFF_SIZE] == 42) usleep(1);
        }

    }
    else {
        memset(data, 0, memsize);
        char* data_ptr;
        int shmid = shmget(key2, BUFF_SIZE + 1, IPC_CREAT | 0666);
        data_ptr = shmat(shmid, NULL, 0);

        for(int i = 0; i < size; i += BUFF_SIZE) {
            while(data_ptr[BUFF_SIZE] != 42) usleep(1);
            memcpy(&data[i], data_ptr, min(size - i, BUFF_SIZE));
            data_ptr[BUFF_SIZE] = 0;
        }

        int output = open("output_shm.mp4", O_WRONLY | O_CREAT, 0644);
        for(int i = 0; i < size; i += BUFF_SIZE)
            write(output, &data[i], min(size - i, BUFF_SIZE));

        shm_dt = get_time() - t0;
        printf("shm: %dus\n", shm_dt);
    }

    if(cid) {
        printf("data size: %d\n", size);
        int logs = open("time_measurements.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
        dprintf(logs, "%d,%d,%d,%d,%d\n", size, BUFF_SIZE, fifo_dt, msg_dt, shm_dt);
    }

    return 0;
}
