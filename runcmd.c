#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_JOBS 64

// job table to track jobs
typedef struct {
    pid_t pid;      //process id
    int job_id;      //job id 1, 2 3..
    char command[MAX_ARGS];     //original command string
    int active;         // 1 if running;

}job;

job jobs[MAX_JOBS];
int next_job_id=1;

int find_free_job_slot(){
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].active==0){
            return i;
        }
    }
    return -1;
}

int add_job(pid_t pid, const char *cmd) {
    int slot = find_free_job_slot();
    if (slot < 0) {
        fprintf(stderr, "Too many jobs\n");
        return -1;
    }
    jobs[slot].pid = pid;
    jobs[slot].job_id = next_job_id++;
    strncpy(jobs[slot].command, cmd, sizeof(jobs[slot].command) - 1);
    jobs[slot].active = 1;
    return slot;
}

void showmsg() {
    printf("-----------------------\n");
    printf("custom shell running\n");
    printf("-----------------------\n");
}

void printcwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
    } else {
        // \033[32m sets text to green, \033[0m resets to default
        printf("\n\033[32m%s\033[0m", cwd);
    }
}


void getinput(char *input ) {
    printf(">>");
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
        printf("\nExiting...\n");
        exit(0);
    }
    input[strcspn(input, "\n")] = 0;
}

void parseInput(char *input, char **args , int *background) {
    int i = 0;
   
    int len = strlen(input);
    if(len>0 && input[len-1]=='&'){
        *background=1;
        input[len-1]='\0';
        while (len > 1 && input[len-2] == ' ') {
            input[len-2] = '\0';
            len--;
        }
    }
    args[i] = strtok(input, " ");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");
    }
}

void executeCommand(char **arg , int background, const char *orig_cmd) {
    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }
    if (child == 0) {
        // for background process redirect input from keybord to bev/null
        if(background){
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        execvp(arg[0], arg);
        perror("execution failed");
        exit(1);
    }

    if (background) {
        // Add to job table
        int child_slot=add_job(child, orig_cmd);
        if(child_slot>=0) printf("[%d] %d\n", jobs[child_slot].job_id, child);
        // Don't wait — return immediately
        // => Zombie child
    } else {
        // Foreground: block until child exits
        waitpid(child, NULL, 0);
    }
}

int handleBuiltIn(char **args) {
    if (args[0] == NULL) return 1;
    if (strcmp(args[0], "exit") == 0) { printf("\033[31mExiting shell...\033[0m\n"); exit(0); }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) printf("Expected argument to \"cd\"\n");
        else if (chdir(args[1]) != 0) perror("cd failed");
        return 1;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("\nSimple Shell Help\n");
        printf("  cd <dir>\n  exit\n  help\n");
        printf("  pipes: ls | grep txt\n");
        printf("  redirect: ls > out, sort < in\n");
        return 1;
    }
    return 0;
}

void sigchld_handler(int sig) {
    // Save and restore errno — handlers can be interrupted mid-syscall
    int saved_errno = errno;

    int status;
    pid_t pid;
    // WNOHANG: don't block if no children have exited
    // -1: reap any child (not a specific pid)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Find this job in the table and mark inactive
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].pid == pid && jobs[i].active) {
                jobs[i].active = 0;
                // Print asynchronously — but ONLY async-signal-safe functions!
                // write() is safe; printf is NOT (see below)
                char msg[128];
                int n = snprintf(msg, sizeof(msg),
                                 "[%d]+ Done  %s\n",
                                 jobs[i].job_id, jobs[i].command);
                if (n > 0) write(STDOUT_FILENO, msg, n);
                break;
            }
        }
    }

    errno = saved_errno;
}


int parsePipe(char *input, char **left, char **right) {
    char *p = strchr(input, '|');
    if (!p) return 0;
    *p = '\0'; p++;
    while (*p == ' ') p++;
    *left = input;
    *right = p;
    return 1;
}

int parseRedirction(char *input, char **left, char **right) {
    char *p = strchr(input, '>');
    if (p) {
        *p = '\0'; p++;
        while (*p == ' ') p++;
        *left = input; *right = p;
        return 1;                       // output redirect
    }
    p = strchr(input, '<');
    if (p) {
        *p = '\0'; p++;
        while (*p == ' ') p++;
        *left = input; *right = p;
        return 2;                       // input redirect
    }
    return 0;                           // no redirect
}

void executeRedirection(char **args1, char **args2, int r, int background, const char *orig_cmd) {
    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }

    if (child == 0) {
        if (background) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        int fd;
        if (r == 1) fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        else        fd = open(args2[0], O_RDONLY);
        if (fd < 0) { perror("open"); exit(1); }
        if (r == 1) dup2(fd, STDOUT_FILENO);
        else        dup2(fd, STDIN_FILENO);
        close(fd);
        execvp(args1[0], args1);
        perror("exec"); exit(1);
    }

    if (background) {
        int slot = add_job(child, orig_cmd);
        if (slot >= 0) printf("[%d] %d\n", jobs[slot].job_id, child);
    } else {
        waitpid(child, NULL, 0);
    }
}

void executePiped(char **args1, char **args2, int background, const char *orig_cmd) {
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); return; }

    pid_t p1 = fork();
    if (p1 == 0) {
        if (background) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(args1[0], args1);
        perror("pipe cmd1"); exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        if (background) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(args2[0], args2);
        perror("pipe cmd2"); exit(1);
    }

    close(fd[0]); close(fd[1]);

    if (background) {
        int slot = add_job(p1, orig_cmd);
        if (slot >= 0) printf("[%d] %d\n", jobs[slot].job_id, p1);
        // p2 is implicitly reaped by SIGCHLD handler since waitpid not called
    } else {
        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);
    }
}


int main(void) {
    showmsg();

    // SIGCHLD handler for reaping background jobs
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    // Ignore SIGTTOU/SIGTTIN
    struct sigaction sa_ign = { .sa_handler = SIG_IGN };
    sigemptyset(&sa_ign.sa_mask);
    sigaction(SIGTTOU, &sa_ign, NULL);
    sigaction(SIGTTIN, &sa_ign, NULL);

    char input[MAX_INPUT];
    char *arg1[MAX_ARGS], *arg2[MAX_ARGS];
    char *left, *right;

    while (1) {
        printcwd();
        getinput(input);

        // Save original command BEFORE parsing mutates it
        char orig_cmd[MAX_INPUT];
        strncpy(orig_cmd, input, sizeof(orig_cmd) - 1);
        orig_cmd[sizeof(orig_cmd) - 1] = '\0';

        int background = 0;

        if (parsePipe(input, &left, &right)) {
            parseInput(left,  arg1, &background);
            parseInput(right, arg2, &background);
            executePiped(arg1, arg2, background, orig_cmd);
        } else {
            int r = parseRedirction(input, &left, &right);
            if (r) {
                parseInput(left,  arg1, &background);
                parseInput(right, arg2, &background);
                executeRedirection(arg1, arg2, r, background, orig_cmd);
            } else {
                parseInput(input, arg1, &background);
                if (!handleBuiltIn(arg1))
                    executeCommand(arg1, background, orig_cmd);
            }
        }
    }
    return 0;
}
