#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdlib.h>

#define MAX_CMDS 20
#define MAX_CMD_LEN 80
#define MAX_CMD_ARGS 6
#define MAX_ARG_LEN (MAX_CMD_LEN/2+1)

#define println(fmt_str, ...) do { printf(fmt_str, ##__VA_ARGS__); fflush(stdout); } while(0)
#define free_s(ptr) do { if((ptr) && *(ptr)) { \
        free(*(ptr)); \
        *(ptr) = NULL; \
    } } while(0)


char* latest_cmd = NULL;
size_t latest_cmd_len = 0;

char* cmd_run = NULL;
char* auto_prompt = NULL;
int cmd_read = 0, should_wait;

char cmds[MAX_CMDS][MAX_CMD_LEN];
char* args[MAX_CMD_ARGS]; // [MAX_ARG_LEN];

int args_cnt = 0;
int head = 0, tail = 0, cmds_cnt = 0;

char* head_v;

static int* child_msg;
int child_proc_ws;

void prompt() {
    println("# osh> ");
    if(auto_prompt) {
        println("%s", auto_prompt);
        auto_prompt = NULL;
    }
}

void add(char* str) {
    if(0 < cmds_cnt && cmds_cnt < MAX_CMDS) {
        head = (head + 1) % MAX_CMDS;
        ++cmds_cnt;
    }
    else if(cmds_cnt == 0) {
        ++cmds_cnt;
    }
    else if(cmds_cnt == MAX_CMDS) {
        head = (head + 1) % MAX_CMDS;
        tail = (tail + 1) % MAX_CMDS;
    }
    else {
        exit(1);
    }

    strcpy(cmds[head], str);
    head_v = cmds[head];
}

int parse(char* str, char* args[]) {
    int i = 0;
    while(str && *str && i < MAX_CMD_ARGS - 1) {
        if(*str == ' ' || *str == '\n') {}
        else {
            args[i] = realloc(args[i], MAX_ARG_LEN * sizeof(char));
            // -- `end` should be out of word bounds after loop
            char* end = str;
            while(end && *end && *end != '\n' && *end != ' ' /*&& *end != '&'*/) ++end;
            strncpy(args[i], str, end - str);
            args[i][end-str] = '\0';
            str = end + 1;
            ++i;
            continue;
        }
        ++str;
    }
    free_s(&args[i]);
    args_cnt = i;
}

void handle_bang() {
    int num = cmds_cnt;
    if(args[0][1] != '!') {
        num = atoi(args[0] + 1);
    }
    if(num > cmds_cnt) {
        cmd_run = NULL;
    }
    else cmd_run = cmds[(tail + num - 1) % MAX_CMDS];
    auto_prompt = cmd_run;
    if(!cmd_run) println("Illegal Index.\n");
}

void handle_history() {
    int i = 0;
    for(int t = tail; i < cmds_cnt; t = (t+1) % MAX_CMDS) {
        println("%d %s", (i+1), cmds[t]);
        ++i;
    }
}

void add_to_history() {
    if(strcmp(cmd_run, cmds[head]) != 0) {
        add(cmd_run);
    }
}

// http://stackoverflow.com/a/22940622
void handle_child(int signum) {
    pid_t pid;
    int child_proc_ws;

    if(!should_wait) {
        while ((pid = waitpid(-1, &child_proc_ws, WNOHANG)) > 0)
        {
        }
        prompt();
    }
    else wait(&child_proc_ws);
}

int main() {
    int running = 1;
    pid_t pid;

    signal(SIGCHLD, handle_child);

    for(int i = 0; i < MAX_CMD_ARGS; ++i) {
        args[i] = malloc(MAX_ARG_LEN * sizeof(char));
    }

    head_v = cmds[head];

    // -- Shared child status code
    child_msg = mmap(NULL, sizeof *child_msg, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    while(running) {
        prompt();

        // -- read command
        if(!cmd_read || !cmd_run) {
            getline(&latest_cmd, &latest_cmd_len, stdin);
            cmd_read = 1;
            cmd_run = latest_cmd;
        }

        parse(cmd_run, args);

        // -- handling internal commands (history, exit, '!')
        if(args_cnt != 0) {
            char* an = args[args_cnt - 1];
            char* a0 = args[0];
            if(strcmp(an, "&") == 0) {
                should_wait = 0;
                // FIXME ! Too much allocation and deallocation
                free_s(&args[args_cnt - 1]);
            }
            else should_wait = 1;
            if(a0[0] == '!') {
                handle_bang();
                cmd_read = 1;
                continue;
            }
            else if(strcmp(a0, "history") == 0) {
                handle_history();
                cmd_read = 0;
                continue;
            }
            else if(strcmp(a0, "exit") == 0) {
                running = 0;
                continue;
            }
            cmd_read = 0;
        }
        else {
            continue;
            cmd_read = 0;
        }

        // -- must invoke external command
        {   
            pid = fork();
            // -- parent
            if(pid > 0) {
                // if(should_wait) {
                //     wait(&child_proc_ws);
                // }
                // else {
                //     waitpid(pid, &child_proc_ws, WNOHANG);
                // }
                add_to_history();
            }
            // -- child
            else if(pid == 0) {
                running = 0;
                int execvp_err = execvp(args[0], args);
                println("Nothing appropriate."
                        " Command '%s' not found or is invalid.\n", args[0]);
                _Exit(EXIT_FAILURE);
            }
            // -- Oh hel~
            else {
                println("Grave: Process creation error.\n");
            }
        }
    }

    return 0;
}
