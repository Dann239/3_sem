#include <unistd.h> //fork pipe read write open close
#include <stdio.h> //printf fgets stdio
#include <string.h> //strcmp strlen
#include <stdlib.h> //malloc
#include <sys/wait.h> //waitpid

#define MAX_LEN __UINT16_MAX__ //maximum length of a message

void swap(int** a, int** b) {
    int* c = *a;
    *a = *b;
    *b = c;
}

typedef struct {
  int* in;
  int* out;
} dpipe_t;

void set_up(dpipe_t* dpipe) {
    dpipe->in = malloc(2 * sizeof(int*));
    dpipe->out = malloc(2 * sizeof(int*)); //allocate memory
    
    pipe(dpipe->in);
    pipe(dpipe->out); //open pipes
}
void assign(dpipe_t* dpipe, int cpid) {
    if(cpid == 0)
        swap(&dpipe->in, &dpipe->out); //for child it works the other way around
    close(dpipe->in[1]); //we don't write into input
    close(dpipe->out[0]); //we don't read from output
}
void send(dpipe_t* dpipe, char* msg, __uint16_t len){
    write(dpipe->out[1], msg, len); //send the message
}
int recieve(dpipe_t* dpipe, char* msg) {
    return read(dpipe->in[0], msg, MAX_LEN); //recieve a message and return its size
}
void cleanup(dpipe_t* dpipe) {
    close(dpipe->in[0]);
    close(dpipe->out[1]);
    free(dpipe->in);
    free(dpipe->out);
}

int is_alive(int ppid, int cpid) {
    if(cpid == 0)
        return ppid == getppid(); //child checks if the parent is alive by validating its pid
    else
        return waitpid(cpid, NULL, WNOHANG) == 0; //parent checks if a child died without halting
}

int main() {
    dpipe_t dpipe;
    set_up(&dpipe);
    int cpid = fork(); //fork
    int ppid = getppid(); //store parent's pid
    assign(&dpipe, cpid);

    char* msg = malloc(MAX_LEN); //allocate memory for the messages
    char* res = malloc(MAX_LEN); //allocate memory for the responses
        
    if(cpid == 0) { //act as child

        while(is_alive(ppid, cpid)) { //recieve messages while the parent is alive
            int len = recieve(&dpipe, msg); //recieve msg

            if(len == 0) //wait until message's size isn't zero
                continue;

            if(!strcmp(msg,"DESTROYTHECHILD")) //DIE on command
                break;

            sprintf(res, "Recieved msg: %s; Length: %i", msg, len); //compile a response

            send(&dpipe, res, strlen(res) + 1); //send a response back through the pipe
        }
        cleanup(&dpipe);
        printf("child terminated\n");
        return 0;
    }
    else { //act as parent
        while(is_alive(ppid, cpid)) { //send messages while the child is alive
            printf("Message: ");
            int len = strlen(fgets(msg, MAX_LEN, stdin)); //get a message from the terminal and store its length
            msg[--len] = 0; //terminate the message with 0 instead of \n

            if(!strcmp("exit",msg)) //exit on command
                break;

            send(&dpipe, msg, len + 1); //send the message with the null-terminator
            sleep(1); //wait for a response
            len = recieve(&dpipe, res);
            if(len)
                printf("Response: %s\n", res);
            else
                printf("No response\n");
        }
        cleanup(&dpipe);
        printf("parent terminated\n");
        return 0;
    }
}