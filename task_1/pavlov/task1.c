#include <unistd.h> //fork
#include <stdio.h> //printf fgets stdin
#include <string.h> //strcmp strtok strlen
#include <stdlib.h> //malloc
#include <sys/wait.h> //waitpid WEXITSTATUS


int main() {
    int arg_max = sysconf(_SC_ARG_MAX); //get the maximum possible length of a command
    char* cmd = malloc(arg_max); //allocate memory for the command
    
    while(1) {
        
        printf("Enter command: ");
        int cmdlen = strlen(fgets(cmd, arg_max, stdin)); //get the "\n\0"-terminated command string and get its length

        while(cmd[cmdlen - 2] == '\\') { //if the input is terminated by '\', override it with ' ' and get the next line
            cmd[cmdlen - 2] = ' ';
            cmdlen--;
            fgets(&cmd[cmdlen], arg_max, stdin);
            cmdlen = strlen(cmd);
        }

        cmd[--cmdlen] = '\0'; //terminate the input with "\0" instead of "\n\0"
        
        if(!strcmp(cmd, "exit")) //die if the command is exit
            return 0;
        
        if(strspn(cmd, " ") == strlen(cmd)) { //ignore commands consisting only of spaces
            printf("Incorrect command, definitely\n");
            continue;
        }

        int c_pid = fork(); //fork into two
    
        int status = c_pid;
        if(c_pid == 0) { //act as child
           
            int argc = (cmd[0] != ' '); //if the first symbol is ' ', start counting from zero

            for(int i = 0; cmd[i]; i++) //count args
                if(cmd[i] == ' ' && cmd[i+1] != ' ' && cmd[i+1] != 0)
                    argc++;

            char** argv = malloc((argc + 1) * sizeof(char*)); //allocate memory for args

            char* name = strtok(cmd, " ");
            argv[0] = name;

            for(int i = 1; i < argc; i++) 
                argv[i] = strtok(NULL," ");

            argv[argc] = 0; //args must be terminated with NULL

            printf("%d\n", argc);
            execvp( //die, replace self with the called command
                name, //pass the name of the command
                argv //pass args
                );
            return 42; //if exec failed, die anyway
        }
        else { //act as parent
            waitpid(c_pid, &status, 0); //wait for the child to die and get its termination status
            status = WEXITSTATUS(status); //convert the full status to just the return value
        }
        if(status == 42) //print return value or report exec failure
            printf("Incorrect command, likely\n");
        else
            printf("Exit status: %d\n", status);
    }
    return 0;
}