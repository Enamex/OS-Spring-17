#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdlib.h>
// #include <math.h>


#define MAX_CMDS 10
#define MAX_CMD_LEN 10
#define MAX_CMD_LN_LEN 140
#define println(fmt_str, ...) do { printf(fmt_str, ##__VA_ARGS__); fflush(stdout); } while(0)
#define free_s(ptr) do { if((ptr) && *(ptr)) { \
        free(*(ptr)); \
        *(ptr) = NULL; \
    } } while(0)

#define max(x, y) (x < y ? y : x)

char* latest_cmd = NULL;
size_t latest_cmd_len = 0;

typedef struct RingQue {
    int s;
    int size;
    char* last;
    char que[MAX_CMDS+1][MAX_CMD_LN_LEN];
} RingQue;

RingQue cmds_hist_ = {.s = -1, .size = 0};
RingQue* cmds_hist = &cmds_hist_;
char* args[MAX_CMD_LEN];

void enqueue(RingQue* q, char* str);
int parse(char* str, int len, char* args[]);

static int* child_exit;

int main() {
    child_exit = mmap(NULL, sizeof *child_exit, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    cmds_hist->last = (char*) cmds_hist->que[0];

    int main_running = 1, take_input = 1;
    int child_status;
    pid_t pid;
    
    while(main_running) {
        *child_exit = 0;
        
        if(take_input) {
            println("osh> ");
            // -- Free recepient string for processing
            latest_cmd = NULL;
            latest_cmd_len = 0;

            // -- Wait for command
            getline(&latest_cmd, &latest_cmd_len, stdin);
        }
        // -- Enqueue latest command at tail of queue (and dequeue oldest entry as necessary)
        if(strcmp(latest_cmd, cmds_hist->last) != 0)
            enqueue(cmds_hist, latest_cmd);
        take_input = 1;
        
        // -- Parse
        int args_len = parse(latest_cmd, latest_cmd_len, args);
        
        // -- Fork for command
        pid = fork();
        
        // -- parent
        if(pid > 0) {
            // println("\n[In parent]\n");
            if(args_len > 0 && *args[args_len-1] == '&') {
                // -- Parent wait if command ends in '&'
                waitpid(pid, &child_status, WNOHANG);
            }
            else {
                wait(&child_status);
                switch(*child_exit) {
                    case 1: {
                        exit(0);
                        break;
                    }
                    case 2: {
                        int num = 0;
                        if(args[0][1] == '!') num = cmds_hist->size - 1;
                        else num = atoi(args[0] + 1);
                        int index = (cmds_hist->s + MAX_CMDS - cmds_hist->size + 1);
                        if(num == cmds_hist->size - 1) {
                            latest_cmd = realloc(latest_cmd, (MAX_CMD_LN_LEN) * sizeof(char));
                            strcpy(latest_cmd, cmds_hist->que[(index + num) % MAX_CMDS]);
                            cmds_hist->size = max(0, cmds_hist->size - 1);
                        }
                        else {
                            latest_cmd = realloc(latest_cmd, (MAX_CMD_LN_LEN) * sizeof(char));
                            // strcpy(cmds_hist->que[cmds_hist->s], cmds_hist->que[(index + num) % MAX_CMDS]);
                            strcpy(latest_cmd, cmds_hist->que[(index + num) % MAX_CMDS]);
                            cmds_hist->size = max(0, cmds_hist->size - 1);
                        }
                        println("osh> %s", latest_cmd);
                        take_input = 0;
                        continue;
                    }
                    break;
                    case 3: {
                        int start = (cmds_hist->s + MAX_CMDS - cmds_hist->size) % MAX_CMDS;
                        for(int i = 0; i < cmds_hist->size; ++i) {
                            println("%d %s", (i + 1), cmds_hist->que[(start + i) % MAX_CMDS]);
                        }
                    }
                    break;
                }
            }
        }
        // -- child
        else if(pid == 0) {
            // println("[In child]\n");
            // -- execute command
            /////
            if(strcmp(args[0], "") == 0) {
                exit(0);
            }
            // -- exit
            else if(strcmp(args[0], "exit") == 0) {
                *child_exit = 1;
                exit(0);
            }
            // -- invoke history
            else if(*args[0] == '!') {
                // -- run last cmd
                *child_exit = 2;
                exit(0);
            }
            else if(strcmp(args[0], "history") == 0) {
                *child_exit = 3;
                exit(0);
            }
            else {
                if(args_len <= 2) args[args_len] = NULL;
                // for(char** a = args; *a; ++a) println("'%s'\n", *a);
                int execvp_fail = execvp(args[0], args);
                // -- handle error
                println("Nothing appropriate. Command not found, is invalid, or"
                        " a disk error has occurred.");
            }
            exit(1);
        }
        // -- failed
        else {
            println("Grave: Error creating process.");
        }
        
    }
    
    return 0;
}

void enqueue(RingQue* q, char* str) {
    // -- Advance ring index
    q->s = ((q->s) + 1) % MAX_CMDS;
    q->last = (char*)q->que[q->s];
    strcpy(q->que[q->s], str);
    if(q->size < MAX_CMDS) ++q->size;
}

int none_of(char c, char cs[], int len) {
    while(len-- >= 0) {
        if(c == cs[len]) return 0;
    }
    return 1;
}

int parse(char* str, int len, char* args[]) {
    int i = 0;
    while(str && *str && i < MAX_CMD_LEN) {
        char* start = str;
        char* end = str;
        if(*str == ' ' || *str == '\n' || *str == '&') {
        }
        else if(*str == '"') {
            while(end && none_of(*end, (char[3]){'"', '\n', '\0'}, 3)) ++end;
            args[i] = realloc(args[i], len * sizeof(char));
            strncpy(args[i], start, end - start);
            args[i][end - start] = '\0';
            str = end;
            ++i;
        }
        else {
            while(end
                    && none_of(*end, (char[5]){' ', '&', '"', '\n', '\0'}, 5)) ++end;
            args[i] = realloc(args[i], len * sizeof(char));
            strncpy(args[i], str, end - start);
            args[i][end - start] = '\0';
            str = end;
            ++i;
        }
        ++str;
    }
    return i;
}
