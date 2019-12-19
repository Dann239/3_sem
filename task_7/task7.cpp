#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
using namespace std;

string regfifo_name = "/tmp/task8server";

int nthreads = 8;
int msgq = msgget(IPC_PRIVATE, 0600);
struct query {
    long mtype;
    int fd;
    int num;
};

string random_string(int len) {
    string res;
    for(int i = 0; i < len; i++) {
        char r = rand() % 36;
        res += r < 10 ? r + '0' : r - 10 + 'a';
    }
    return res;
}

void send_data(int descriptor, int number) {
    string secret_information[] = {
        "no such information, or not enough clearance",
        "illuminati conspiracy is real",
        "jeffrey epstein didnt kill himself",
        "israel is not a legitimate state",
        "42",
        "global warming is a hoax",
        "usa is sponsoring global terrorism",
        "change da world. my final message. goodb ye",
        "birds arent real",
        "earth is flat",
        "pi = 3",
        "evolution is a carefully constructed lie",
        "peace was never an option"
    };

    sleep(3); //perform sophisticated and lengthy security checks to ensure that the recipient is a trusted party
   
    if(number < 1 || number >= sizeof(secret_information) / sizeof(string))
        number = 0;
    
    dprintf(descriptor, "%s\n", secret_information[number].c_str());
}

void thread_routine() {
    while(true) {
        query q;
        msgrcv(msgq, &q, sizeof(int) * 2, 1, 0);
        send_data(q.fd, q.num);
    }
}

void server() {
    vector<thread> threads;
    for(int i = 0; i < nthreads; i++)
        threads.push_back(thread(thread_routine));
    signal(SIGPIPE, SIG_IGN);
    mknod(regfifo_name.c_str(), S_IFIFO | 0600, 0);
    vector<pollfd> fds;
    vector<int> outs;
    fds.push_back({open(regfifo_name.c_str(), O_RDWR), POLLIN | POLLERR | POLLHUP, 0});
    outs.push_back(-1);
    cerr << "FactSpitter 1.0.2 FIFO Server Online!" << endl;
    while(true) {
        fds[0].revents = 0;
        poll(&fds[0], fds.size(), -1);
        if(fds[0].revents & POLLIN) {
            char command[100], arg1[100], arg2[100];
            fscanf(fdopen(fds[0].fd, "r"), "%s %s %s", command, arg1, arg2);
            if(strcmp(command, "REGISTER"))
                cerr << "command is not 'register', whats the point?" << endl;
            else if(rand() % 10){
                fds.push_back({open(arg1, O_RDONLY), POLLIN | POLLHUP, 0});
                outs.push_back(open(arg2, O_WRONLY));
                dprintf(outs.back(), "ACK\n");
                cerr << "new client registered" << endl;
            } else {
                int out = open(arg2, O_WRONLY);
                dprintf(out, "NAK. Get rekd\n");
                cerr << "not feeling like registering this one" << endl;
                close(out);
            }
        }
        for(int i = 1; i < fds.size(); i++)
            if(fds[i].revents & POLLIN) {
                char command[100];
                int arg;
                fscanf(fdopen(fds[i].fd, "r"), "%s %d", command, &arg);
                if(strcmp(command, "GET")) {
                    cerr << "command is not 'get', are you retarded?" << endl;
                    continue;
                }
                query q = {1, outs[i], arg};
                msgsnd(msgq, &q, sizeof(int) * 2, 0);
                cerr << "a file request queried" << endl;
            } else if(fds[i].revents & POLLHUP) {
                close(fds[i].fd);
                close(outs[i]);
                fds.erase(fds.begin() + i);
                outs.erase(outs.begin() + i);
                i--;
                cerr << "a client dieded" << endl;
            }
    }
}

void chaneller(int fd) {
    int ppid = getppid();
    while(true) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        char buff[1024];
        int size;
        while((size = read(fd,buff,1024)) < 1) usleep(1);
        write(STDOUT_FILENO, buff, size);
        if(getppid() != ppid)
            exit(0);
    }
}

void client() {
    cerr << "Welcome to FactSpitter 1.0.2 FIFO client!" << endl;
    string in = "/tmp/" + random_string(6);
    string out = "/tmp/" + random_string(6);
    mknod(in.c_str(), S_IFIFO | 0600, 0);
    mknod(out.c_str(), S_IFIFO | 0600, 0);
    if(!fork())
        chaneller(open(in.c_str(), O_RDONLY));
    int serv = open(regfifo_name.c_str(), O_WRONLY);
    dprintf(serv, "REGISTER %s %s\n", out.c_str(), in.c_str());
    int fd_out = open(out.c_str(), O_WRONLY);
    while(true) {
        int num = rand() % 15;
        dprintf(fd_out, "GET %d\n", num);
        sleep(4);
    }
}

long long mtime() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    if(argc == 1) {
        cout << "need args: server or client" << endl;
        return 0;
    }
    string args = argv[1];
    srand(mtime());
    if(args == "server")
        server();
    if(args == "client")
        client();
}