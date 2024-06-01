#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

void sigint_handler(int sig) {
    printf("\nYou typed Control-C!\n");
}

int main() {
    char command[1024], lastCommand[1024] = "";
    char *token;
    char *outfile;
    int i, fd, amper, redirect, retid, status, redirect_stderr, append, lastStatus = 0;
    char *argv[10];
    char prompt[1024] = "hello: "; // Default prompt

    signal(SIGINT, sigint_handler);  // Handle Control-C

    while (1) {
        printf("%s", prompt); // Use the current prompt
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';

                // Store the command before parsing for !! execution
        if (strcmp(command, "!!") != 0) {
            strcpy(lastCommand, command); // Update last command
        }

        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        argv[i] = NULL;

        /* Built-in commands */
        if (strcmp(argv[0], "!!") == 0) {
            if (strlen(lastCommand) == 0) {
                printf("No commands in history.\n");
                continue;
            }
            printf("%s\n", lastCommand);
            strcpy(command, lastCommand); // Copy last command back to command
            // Reparse the repeated command
            i = 0;
            token = strtok(command, " ");
            while (token != NULL) {
                argv[i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            argv[i] = NULL;
        }

        if (argv[0] != NULL && strcmp(argv[0], "prompt") == 0 && strcmp(argv[1], "=") == 0 && argv[2] != NULL) {
            strcpy(prompt, argv[2]);
            strcat(prompt, ": "); // Add colon and space to the new prompt
            continue;
        }
        if (argv[0] != NULL && strcmp(argv[0], "echo") == 0) {
            if (argv[1] != NULL && strcmp(argv[1], "$?") == 0) {
                printf("%d\n", lastStatus);
            } else {
                for (int j = 1; argv[j] != NULL; j++) {
                    printf("%s ", argv[j]);
                }
                printf("\n");
            }
            continue;
        }
        if (strcmp(argv[0], "cd") == 0) {
            if (argv[1] == NULL || chdir(argv[1]) != 0) {
                perror("cd failed");
            }
            continue;
        }
        if (strcmp(argv[0], "quit") == 0) {
            printf("Exiting shell.\n");
            exit(0);
        }

        /* reset control variables for each command */
        amper = redirect = redirect_stderr = append = 0;
        outfile = NULL;

        /* check for & (background), > (redirect stdout), 2> (redirect stderr), and >> (append) */
        for (int j = 0; argv[j]; j++) {
            if (!strcmp(argv[j], "&")) {
                amper = 1;
                argv[j] = NULL;
                break;
            } else if (!strcmp(argv[j], ">")) {
                redirect = 1;
                outfile = argv[j + 1];
                argv[j] = NULL;
            } else if (!strcmp(argv[j], "2>")) {
                redirect_stderr = 1;
                outfile = argv[j + 1];
                argv[j] = NULL;
            } else if (!strcmp(argv[j], ">>")) {
                append = 1;
                outfile = argv[j + 1];
                argv[j] = NULL;
            }
        }

       /* for commands not part of the shell command language */
        if (fork() == 0) {
            /* handle redirection of stdout */
            if (redirect) {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            /* handle redirection of stderr */
            if (redirect_stderr) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                close(STDERR_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            /* handle appending to file */
            if (append) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(argv[0], argv);
            /* If execvp fails, print an error message directly to stderr */
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(127); // Exit with 127 if exec fails
        }

        /* parent continues here */
        if (!amper) {
            retid = wait(&status);
            if (WIFEXITED(status)) {
                lastStatus = WEXITSTATUS(status); // Capture the exit status of the command
            } else {
                lastStatus = 1; // Non-zero status for abnormal exit
            }
        }
    }
    return 0;
}
