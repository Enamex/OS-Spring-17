#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

// #include <stdbool.h>

#define MAX_CMDS 10
#define MAX_CMD_LEN 10
#define println(fmt_str, ...) do { printf(fmt_str, ##__VA_ARGS__); fflush(stdout); } while(0)
#define free_s(ptr) do { if((ptr) && *(ptr)) { \
        free(*(ptr)); \
        *(ptr) = NULL; \
    } } while(0)

char* latest_cmd = NULL;
size_t latest_cmd_len = 0;

typedef struct RingQue {
    int s, e;
    char* que[MAX_CMDS+1];
} RingQue;

RingQue cmds_hist_ = {.s = 0, .e = 0};
RingQue* cmds_hist = &cmds_hist_;
char* args[MAX_CMD_LEN];

void enqueue(RingQue* q, char* str);
// void free_s(void** ptr);
int parse(char* str, int len, char* args[]);


int main() {
    // char* vs[] = {"ls"};
    // execvp("man", vs);

    int main_running = 1;
    int child_status;
    pid_t pid;
    
    while(main_running) {
        println("osh> ");

        // -- Free recepient string for processing
        latest_cmd = NULL;
        latest_cmd_len = 0;
        getline(&latest_cmd, &latest_cmd_len, stdin);
        
        // -- Enqueue latest command at tail of queue (and dequeue oldest entry as necessary)
        enqueue(cmds_hist, latest_cmd);
        
        // -- Parse
        int args_len = parse(latest_cmd, latest_cmd_len, args);
        
        // -- Execute commands
        pid = fork();
        
        // -- parent
        if(pid > 0) {
            // println("\n[In parent]\n");
            if(*args[args_len-1] == '&') {
                // -- Parent wait if command ends in '&'
                waitpid(pid, &child_status, 0);
            }
            wait(&child_status);
        }
        // -- child
        else if(pid == 0) {
            // println("\n[In child]\n");
            // -- execute command
            /////
            // -- invoke history
            if(strcmp(args[0], "history") == 0) {
                
            }
            else {
                // if(args_len <= 2) args[args_len] = "\0";
                for(char** a = args; *a; ++a) println("'%s'\n", *a);
                int execvp_fail = execvp(args[0], args);
                // -- handle error
                println("Nothing appropriate. Command not found, is invalid, or"
                        " a disk error has occurred.");
                println("\n{%d}\n", execvp_fail);
            }
            exit(1);
        }
        // -- failed
        else {
            println("Grave: Error creating process.");
        }
        
    }
    
    // free(latest_cmd);
    
    return 0;
}

void enqueue(RingQue* q, char* str) {
    // -- Advance ring index
    q->s = ((q->s) + 1) % MAX_CMDS;
    // -- Free current tenant of start ptr
    free_s(&q->que[q->s]);
    q->que[q->s] = str;
}

int parse(char* str, int len, char* args[]) {
    int i = 0;
    while(str && *str && i < MAX_CMD_LEN - 1) {
        char* start = str;
        char* end = str;
        if(*str == ' ' || *str == '\n') {
        }
        else if(*str == '"') {
            while(end && *end && *end != '"' && *end != '\n') ++end;
            args[i] = realloc(args[i], len * sizeof(char));
            strncpy(args[i], start, end - start);
            args[i][end - start] = '\0';
            str = end - 1;
            ++i;
        }
        else {
            while(end && *end && *end != ' '
                    && *end != '"' && *end != '\n') ++end;
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
