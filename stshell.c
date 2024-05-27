#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void sigint_handler(int sig)
{
    printf("\nEnter 'exit' to exit stshell program(get sig=%d).\n", sig);
    signal(SIGINT, sigint_handler);
}

/* divide the command into argv arguments, ends with NULL */
void get_command(char *command, char **argv)
{
    char *separate_command = strtok(command, " \n");
    int index = 0;
    while (separate_command != NULL)
    {
        argv[index] = separate_command;
        separate_command = strtok(NULL, " \n");
        index++;
    }
    argv[index] = NULL;
}

 /* return -1 if not exist */
int char_index(char **argv, char *char_keyword)
{
    int index = 0;
    while (argv[index] != NULL)
    {
        if (strcmp(argv[index], char_keyword) == 0)
        {
            return index;
        }
        index++;
    }
    return -1;
}

/* get the commands between the pipelines and execute */
void pipeline_handler(int pipe_count, char **argv)
{
    int pipe_file_descriptors[2 * pipe_count];
    for (int i = 0; i < pipe_count; i++)
    {
        if (pipe(pipe_file_descriptors + i * 2) < 0)
        {
            perror("cannot pipe, failed.");
            exit(1);
        }
    }

    int index_command = 0;
    int output_fd = -1;
    int is_append = 0;

    for (int i = 0; i <= pipe_count; i++)
    {
        char *command_argv[10];
        int index_argument = 0;
        while (argv[index_command] != NULL && strcmp(argv[index_command], "|") != 0)
        {
            if (strcmp(argv[index_command], ">") == 0 || strcmp(argv[index_command], ">>") == 0)
            {
                if (argv[index_command + 1] == NULL)
                {
                    fprintf(stderr, "Error: Missing output file name\n");
                    exit(1);
                }

                if (strcmp(argv[index_command], ">>") == 0)
                {
                    is_append = 1;
                }

                argv[index_command++] = NULL;
                int flags = O_WRONLY | O_CREAT | (is_append ? O_APPEND : O_TRUNC);
                output_fd = open(argv[index_command], flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_fd < 0)
                {
                    perror("Error opening output file");
                    exit(1);
                }
                break;
            }

            command_argv[index_argument++] = argv[index_command++];
        }
        command_argv[index_argument] = NULL;
        index_command++; // to skip the pipeline "|"

        pid_t pid = fork();
        if (pid == 0)
        {
            if (i != 0)
            {
                dup2(pipe_file_descriptors[(i - 1) * 2], STDIN_FILENO);
            }

            if (i != pipe_count)
            {
                dup2(pipe_file_descriptors[i * 2 + 1], STDOUT_FILENO);
            }
            else if (output_fd != -1)
            {
                dup2(output_fd, STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * pipe_count; j++)
            {
                close(pipe_file_descriptors[j]);
            }

            if (output_fd != -1)
            {
                close(output_fd);
            }

            execvp(command_argv[0], command_argv);
            perror("Error executing command");
            exit(1);
        }
    }

    for (int i = 0; i < 2 * pipe_count; i++)
    {
        close(pipe_file_descriptors[i]);
    }

    if (output_fd != -1)
    {
        close(output_fd);
    }

    for (int i = 0; i <= pipe_count; i++)
    {
        wait(NULL);
    }
}


int main()
{
    char command[1024];
    char *argv[512];
    pid_t child_pid;
    int status;

    signal(SIGINT, sigint_handler);

    while (1)
    {
        printf("stshell$ ");
        fgets(command, 1024, stdin);
        get_command(command, argv);

        if (argv[0] == NULL)
            continue;

        if (strcmp(argv[0], "exit") == 0)
            break;

        int redirect_out = char_index(argv, ">");
        int append_out = char_index(argv, ">>");
        int pipe_pos = char_index(argv, "|");

        if ((redirect_out != -1 || append_out != -1) && (pipe_pos != -1))
        {
            int pipe_count = 0;
            for (int i = 0; argv[i] != NULL; i++)
            {
                if (strcmp(argv[i], "|") == 0)
                {
                    pipe_count++;
                }
            }

            pipeline_handler(pipe_count, argv);
        }
        else if (redirect_out != -1 || append_out != -1)
        {
            int file_descriptor;
            if (redirect_out != -1)
            {
                argv[redirect_out] = NULL;
                file_descriptor = open(argv[redirect_out + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            else
            {
                argv[append_out] = NULL;
                file_descriptor = open(argv[append_out + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            }

            if (file_descriptor < 0)
            {
                perror("Error opening file");
                continue;
            }

            child_pid = fork();
            if (child_pid == 0)
            {
                dup2(file_descriptor, STDOUT_FILENO);
                close(file_descriptor);
                printf("before execvp: argv[0]=%s\n", argv[0]);
                execvp(argv[0], argv);
                printf("after execvp: argv[0]=%s\n", argv[0]);
                perror("Error executing command");
                exit(1);
            }
            else
            {
                close(file_descriptor);
                printf("before waitpid: child=%d\n", child_pid);
                waitpid(child_pid, &status, 0);
                printf("after waitpid: child=%d\n", child_pid);
            }
        }
        else if (pipe_pos != -1)
        {
            int pipe_count = 0;
            for (int i = 0; argv[i] != NULL; i++)
            {
                if (strcmp(argv[i], "|") == 0)
                {
                    pipe_count++;
                }
            }

            pipeline_handler(pipe_count, argv);
        }
        else
        {
            child_pid = fork();
            if (child_pid == 0)
            {
                execvp(argv[0], argv);
                perror("Error executing command");
                exit(1);
            }
            else
            {
                waitpid(child_pid, &status, 0);
            }
        }
    }

    return 0;
}
