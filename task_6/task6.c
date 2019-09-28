#define _GNU_SOURCE
#include <stdio.h> //printf
#include <signal.h> //kill
#include <unistd.h> //fork sleep read write
#include <stdlib.h> //system
#include <fcntl.h> //S_IFIFO open close
#include <sys/stat.h> //mknod
#include <string.h> //strlen strcmp memcpy
#include <limits.h> //PATH_MAX
#include <sys/inotify.h> //inotify

#define FIFO_CMD "/tmp/custom_daemon_fifo_in" //fifo for passing commands to daemon
#define FIFO_LOG "/tmp/custom_daemon_fifo_out" //fifo for getting response from daemon

char path[PATH_MAX] = ".";


void custom_daemon() {
    int log = open(FIFO_LOG, O_WRONLY | O_APPEND);
    int cpid = fork();
    if(cpid) { //act as parent - listen for commands from fifo
        dprintf(log, "new daemon started\n");
        int cmd = open(FIFO_CMD, O_RDONLY);
        while(1) {
            char c = 0;
            if(read(cmd, &c, 1) <= 0)
                continue;
            switch(c) { //read the command and execute it
            case 'k': //kill on k
                dprintf(log, "daemon terminated\n");
                kill(cpid, SIGINT);
                exit(0);

            case 'p': //give PID on p
                dprintf(log, "daemon parent PID is %d; child PID is %d\n", getpid(), cpid);
                break;

            case 'd': //give path on d
                dprintf(log, "daemon home: %s\n", path);
                break;
            }
        }
    }
    else { //act as child - do all the job
        
    }
}

void send_command(char c) { //send a command to the daemon
    int cmd = open(FIFO_CMD, O_WRONLY | O_NONBLOCK | O_APPEND); //open fifo without halting if there is no listening counterpart
    write(cmd, &c, 1); //send the message or at least attempt to
}

void start_daemon() { //start the daemon
        int cpid = fork(); //fork, 
        if(cpid) {
            sleep(2); //wait for the daemon to start, then leave
            return;
        }
        send_command('k'); //kill the previous daemon if it's present
        sleep(1); //wait for the daemon to die
        custom_daemon(); //startup the new daemon
}

void dump_log(int log) { //print accumulated logs
    char* bulk = malloc(2000000);
    read(log, bulk, 2000000);
    printf("%s", bulk);
    free(bulk);
}

int main(int argc, char** argv) {
    if(argc == 1) {
        printf("\
no arguments passed, exiting\n\
to start the daemon in your current directory, pass \'start\' as the first arg\n\
to start the daemon in any other directory, pass its path as an arg\n\
to get daemon's PID, use -p\n\
to kill daemon, use -k\n\
to get daemon's home directory, use -d\n
do not rename the daemon's home direcory\n");
        return 0;
    }
    mknod(FIFO_LOG, S_IFIFO | 0644, 0);
    mknod(FIFO_CMD, S_IFIFO | 0644, 0); //initialize fifos
    int log = open(FIFO_LOG, O_RDONLY | O_NONBLOCK);
    if(argc > 1 && !strcmp(argv[1], "start")) //start daemon if there is a start command
        start_daemon();
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') { //if an arg starts with '-', send following letters as commands
            for(int j = 1; j < strlen(argv[i]); j++)
                send_command(argv[i][j]);
        } else if(strcmp(argv[i],"start")) { //if the arg doesn't start with '-' and isn't "start" then set it as the new path and restart the daemon
            sprintf(path, "%s", argv[i]);
            start_daemon();
        }
    }
    sleep(1);
    dump_log(log); //wait and dump logs
}
