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
#include <errno.h>
#include <dirent.h>
#include <time.h>

char path_global[PATH_MAX] = "/home/palych/Documents/info/test";

int inotify_fds[0x1000];
int inotify_wds[0x1000];
char* inotify_paths[0x1000];
int inotify_size = 0;

int time0 = 0;

int restoration_script;
int log_file;

int get_time() {
    return time(NULL) - time0;
}

char* rand_str() {
    static char res[11];
    for(int i = 0; i < 10; i++){
        int r = rand() % 36;
        res[i] = r > 9 ? 'a' + r - 10 : '0' + r;
    }
    res[10] = 0;
    return res;
}

void restoration_append(char* cmd) {
    dprintf(restoration_script, "if [ $1 -gt %d ]\nthen\n%sfi\n", get_time(), cmd);
}

void push(char* path) {
    inotify_fds[inotify_size] = inotify_init1(IN_NONBLOCK);
    inotify_wds[inotify_size] = inotify_add_watch(inotify_fds[inotify_size], path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF);
    
    inotify_paths[inotify_size] = malloc(strlen(path) + 1);
    strcpy(inotify_paths[inotify_size], path);
    inotify_size++;
}

int is_dir(char* path) {
    struct stat info;           
    stat(path, &info);
    return S_ISDIR(info.st_mode);
}

void net(char* name) {
    DIR *dir = opendir(name);
    push(name);
                    
    if(dir) {
        char path[PATH_MAX], *end_ptr = path;
        strcpy(path, name);                
        
        end_ptr = &path[strlen(path)]; 
        *(end_ptr++) = '/';
          
        struct dirent *e;
        while (e = readdir(dir)) { 
            char* dname = e -> d_name;
            if(e->d_name[0] == '.')
                continue;
            strcpy(end_ptr, e -> d_name);
            if (is_dir(path))
                net(path);
        }
    }
}

int is_subdir(char* src, char* path) {
    int i = 0;
    if(strlen(path) <= strlen(src))
        return 0;
    for(; i < strlen(src); i++)
        if(src[i] != path[i])
            return 0;
    return path[strlen(src)] == '/';
}

void subdir_move(char* src, char** file, char* trg) {
    char* path = *file;
    if(!is_subdir(src,path))
        return;
    char* newpath = malloc(strlen(path) - strlen(src) + strlen(trg) - 1);
    newpath[0] = 0;
    strcat(newpath, trg);
    strcat(newpath, &(path[strlen(src)]));
    free(*file);
    *file = newpath;
}

void movement(char* src, char* trg) {
    dprintf(log_file, "%s moved to %s at %ds\n", src, trg, get_time());
    for(int i = 0; i < inotify_size; i++)
        subdir_move(src, &inotify_paths[i], trg);
    
    char cmd[0x2000] = "";
    sprintf(cmd, "mv -f \"$DAEMONIC_PATH%s\" \"$DAEMONIC_PATH%s\"\n", &src[strlen(path_global)], &trg[strlen(path_global)]);
    system(cmd);
    restoration_append(cmd);
}
void deletion(char* trg) {
    dprintf(log_file, "%s deleted at %ds\n", trg, get_time());
    for(int i = 0; i < inotify_size; i++)
        if(is_subdir(trg, inotify_paths[i]) || !strcmp(trg, inotify_paths[i])) {
            inotify_rm_watch(inotify_fds[i], inotify_wds[i]);
        }
    
    char cmd[0x2000] = "";
    sprintf(cmd, "rm -rf \"$DAEMONIC_PATH%s\"\n", &trg[strlen(path_global)]);
    system(cmd);
    restoration_append(cmd);
}
void creation(char* trg) {
    dprintf(log_file, "%s created at %ds\n", trg, get_time());
    if(is_dir(trg))
        net(trg);
    char* rstr = rand_str();
    setenv("RSTR", rstr, 1);
    setenv("GTRG", trg, 1);
    
    system("cp -rf \"$GTRG\" \"/tmp/daemonic/created/$RSTR\"");
    
    char cmd[0x2000] = "";
    sprintf(cmd, "cp -rf \"/tmp/daemonic/created/%s\" \"$DAEMONIC_PATH%s\"\n", rstr, &trg[strlen(path_global)]);
    system(cmd);
    restoration_append(cmd);
}
void modification(char* trg) {
    dprintf(log_file, "%s modified at %ds\n", trg, get_time());
    char* rstr = rand_str();
    setenv("RSTR", rstr, 1);
    setenv("GTRG", trg, 1);
    setenv("RTRG", &trg[strlen(path_global)], 1);
    
    system("diff -a \"$DAEMONIC_PATH$RTRG\" \"$GTRG\" > /tmp/daemonic/diffs/$RSTR");

    char cmd[0x2000] = "";
    sprintf(cmd, "patch -s \"$DAEMONIC_PATH%s\" \"/tmp/daemonic/diffs/%s\"\n", &trg[strlen(path_global)], rstr);
    system(cmd);
    restoration_append(cmd);
}

void scan() {
    char buff[0x4000];
    char str[0x1000];

    static char storage[0x80][0x1000];
    int cookies[0x80];
    int storage_size = 0;

    struct inotify_event* event = (void*)buff;
    int data_size;
    for(int i = 0; i < inotify_size; i++) {
        data_size = read(inotify_fds[i], buff, 0x4000);
        if(data_size <= 0)
            continue;
        for(int j = 0; j < data_size; j+=0x10) {
            struct inotify_event* e = (void*)&buff[j];
            j += e->len;
            str[0] = 0;
            strcpy(str, inotify_paths[i]);
            strcat(str, "/");
            if(e->len)
                strcat(str, e->name);
            int pair_located;
            for(unsigned int k = 0, m = 1; k < 32; k++, m <<= 1)
                switch(e->mask & m) {
                    case IN_CREATE:
                        creation(str);
                        break;

                    case IN_DELETE:
                        deletion(str);
                        break;

                    case IN_MODIFY:
                        modification(str);
                        break;

                    case IN_MOVED_FROM:
                        pair_located = 0;
                        for(int m = 0; m < storage_size; m++) {
                            if(cookies[m] == e->cookie) {
                                movement(str, storage[m]);
                                pair_located = 1;
                                cookies[m] = 0;
                            }
                        }
                        if(!pair_located) {
                            cookies[storage_size] = -e->cookie;
                            strcpy(storage[storage_size], str);
                            storage_size++;
                        }
                        break;

                    case IN_MOVE_SELF:
                        inotify_rm_watch(inotify_fds[i], inotify_wds[i]);
                        break;

                    case IN_MOVED_TO:
                        if(is_dir(str))
                            net(str);

                        pair_located = 0;
                        for(int m = 0; m < storage_size; m++) {
                            if(cookies[m] == -e->cookie) {
                                movement(storage[m], str);
                                pair_located = 1;
                                cookies[m] = 0;
                            }
                        }
                        if(!pair_located) {
                            cookies[storage_size] = e->cookie;
                            strcpy(storage[storage_size], str);
                            storage_size++;
                        }
                        break;

                    case IN_IGNORED:
                    case IN_ISDIR:
                        continue;

                    case 0:
                        break;

                    default:
                        printf("UNKNOWN ACTION %d %d %d\n", e->mask, m, e->mask & m);
                }
        }
    }
    for(int k = 0; k < storage_size; k++) {
        if(cookies[k] > 0)
            creation(storage[k]);
        if(cookies[k] < 0)
            deletion(storage[k]);
    }
}

int main() {
    time0 = time(NULL);

    srand(getpid());
    
    setenv("DAEMONIC_PATH", path_global, 1);
    system("\
rm -rf /tmp/daemonic ; \
mkdir /tmp/daemonic /tmp/daemonic/diffs /tmp/daemonic/created ;\
cp -rf $DAEMONIC_PATH /tmp/daemonic/init ;\
cp -rf $DAEMONIC_PATH /tmp/daemonic/current");

    setenv("DAEMONIC_PATH", "/tmp/daemonic/current", 1);
    restoration_script = open("/tmp/daemonic/restoration.sh", O_WRONLY | O_CREAT, 0777);
    dprintf(restoration_script, "if [ \"$DAEMONIC_PATH\" == \"\" ] || [ \"$1\" == \"\" ]\nthen\nexit\nfi\n");
    dprintf(restoration_script, "rm -rf $DAEMONIC_PATH/*\ncp -rf /tmp/daemonic/init/* -t $DAEMONIC_PATH\n");

    log_file = open("/tmp/daemonic/logfile.log", O_WRONLY | O_CREAT, 0777);

    net(path_global);
    while(1) {
        scan();
        usleep(1000);
    }
}
