#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

void showmsg() {
    printf("-----------------------\n");
    printf("custom shell running\n");
    printf("-----------------------\n");
}

void printcwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) perror("getcwd failed");
    else printf("\n%s", cwd);
}

void getinput(char *input) {
    printf(">>");
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
        printf("\nExiting...\n");
        exit(0);
    }
    input[strcspn(input, "\n")] = 0;
}

void parseInput(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " ");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");
    }
}

void executeCommand(char **arg) {
    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }
    if (child == 0) {
        execvp(arg[0], arg);
        perror("execution failed");
        exit(1);
    }
    wait(NULL);
}

int handleBuiltIn(char **args) {
    if (args[0] == NULL) return 1;
    if (strcmp(args[0], "exit") == 0) { printf("Exiting shell...\n"); exit(0); }
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

void executeRedirection(char **args1, char **args2, int r) {
    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }

    if (child == 0) {
        int fd;
        if (r == 1) {
            fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else { /* r == 2 */
            fd = open(args2[0], O_RDONLY);
        }
        if (fd < 0) { perror("open"); exit(1); }

        if (r == 1) dup2(fd, STDOUT_FILENO);
        else        dup2(fd, STDIN_FILENO);
        close(fd);

        execvp(args1[0], args1);
        perror("exec"); exit(1);
    }
    wait(NULL);
}

void executePiped(char **args1, char **args2) {
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); return; }

    pid_t p1 = fork();
    if (p1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(args1[0], args1);
        perror("pipe cmd1"); exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(args2[0], args2);
        perror("pipe cmd2"); exit(1);
    }

    close(fd[0]); close(fd[1]);
    wait(NULL); wait(NULL);
}

int main(void) {
    showmsg();
    char input[MAX_INPUT];
    char *arg1[MAX_ARGS], *arg2[MAX_ARGS];
    char *left, *right;

    while (1) {
        printcwd();
        getinput(input);

        if (parsePipe(input, &left, &right)) {
            parseInput(left,  arg1);
            parseInput(right, arg2);
            executePiped(arg1, arg2);
        } else {
            int r = parseRedirction(input, &left, &right);
            if (r) {
                parseInput(left,  arg1);
                parseInput(right, arg2);
                executeRedirection(arg1, arg2, r);
            } else {
                parseInput(input, arg1);
                if (!handleBuiltIn(arg1))
                    executeCommand(arg1);
            }
        }
    }
    return 0;
}
