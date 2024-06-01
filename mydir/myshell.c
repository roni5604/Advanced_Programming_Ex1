#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <deque>
#include <map>
#include <string>
#include <iostream>

#define MAXLINE 1024

using namespace std;

// Function prototypes
void execute_command(char *cmd);
void execute_single_command(char *cmd);
void handle_sigint(int sig);

// Global variables
char prompt[MAXLINE] = "hello";
char last_command[MAXLINE] = "";
int last_status = 0;
map<string, string> variables;
deque<string> history;

int main() {
    char line[MAXLINE];
    char *cmd;

    signal(SIGINT, handle_sigint);

    while (1) {
        printf("%s: ", prompt);
        if (!fgets(line, MAXLINE, stdin)) break;

        if ((cmd = strchr(line, '\n')) != NULL) *cmd = '\0';

        if (strlen(line) == 0) continue;

        if (strncmp(line, "prompt = ", 9) == 0) {
            strncpy(prompt, line + 9, MAXLINE - 1);
            prompt[MAXLINE - 1] = '\0';
        } else {
            execute_command(line);
        }
    }

    return 0;
}

void handle_sigint(int sig) {
    printf("\nYou typed Control-C!\n");
    fflush(stdout);
}

void execute_command(char *cmd) {
    // Handle !! (repeat last command)
    if (strcmp(cmd, "!!") == 0) {
        if (strlen(last_command) == 0) {
            printf("No commands in history.\n");
            return;
        }
        strcpy(cmd, last_command);
        printf("%s\n", cmd);
    } else {
        strcpy(last_command, cmd);
    }

    // Add to history
    history.push_back(cmd);
    if (history.size() > 20) {
        history.pop_front();
    }

    // Tokenize by '|'
    char *commands[MAXLINE / 2 + 1];
    int cmd_count = 0;
    char *token = strtok(cmd, "|");
    while (token != NULL) {
        commands[cmd_count++] = token;
        token = strtok(NULL, "|");
    }

    int pipe_fd[2], fd_in = 0;
    for (int i = 0; i < cmd_count; i++) {
        pipe(pipe_fd);
        if (fork() == 0) {
            dup2(fd_in, 0);
            if (i < cmd_count - 1) dup2(pipe_fd[1], 1);
            close(pipe_fd[0]);
            execute_single_command(commands[i]);
            exit(0);
        } else {
            wait(NULL);
            close(pipe_fd[1]);
            fd_in = pipe_fd[0];
        }
    }
}

void execute_single_command(char *cmd) {
    pid_t pid;
    int status;
    char *args[MAXLINE / 2 + 1]; // command line arguments
    int arg_count = 0;
    char *token;

    // Tokenize the command string
    token = strtok(cmd, " ");
    while (token != NULL) {
        if (token[0] == '$') {
            string var_name = token + 1;
            if (variables.find(var_name) != variables.end()) {
                token = (char *)variables[var_name].c_str();
            }
        }
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    // Built-in commands
    if (strcmp(args[0], "cd") == 0) {
        if (chdir(args[1]) != 0) {
            perror("chdir");
        }
        return;
    }

    if (strcmp(args[0], "echo") == 0) {
        if (arg_count == 2 && strcmp(args[1], "$?") == 0) {
            printf("%d\n", last_status);
        } else {
            for (int i = 1; i < arg_count; i++) {
                printf("%s ", args[i]);
            }
            printf("\n");
        }
        return;
    }

    if (strcmp(args[0], "quit") == 0) {
        exit(0);
    }

    if (strncmp(cmd, "read ", 5) == 0) {
        char *var_name = strtok(cmd + 5, " ");
        char value[MAXLINE];
        printf("Enter a string: ");
        fgets(value, MAXLINE, stdin);
        if (char *newline = strchr(value, '\n')) *newline = '\0';
        variables[var_name] = value;
        return;
    }

    if (strstr(cmd, "=") != NULL) {
        char *var = strtok(cmd, "=");
        char *value = strtok(NULL, "=");
        variables[var] = value;
        return;
    }

    // Check for output redirection
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], "2>") == 0 && i + 1 < arg_count) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open");
                return;
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
            args[i] = NULL;
            break;
        }
        if (strcmp(args[i], ">>") == 0 && i + 1 < arg_count) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                return;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            break;
        }
    }

    if ((pid = fork()) < 0) {
        perror("fork");
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else {
        while (wait(&status) != pid);
        last_status = WEXITSTATUS(status);
    }
}