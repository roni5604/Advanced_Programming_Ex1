#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>

#define MAX_VARIABLES 100
#define HISTORY_SIZE 100

// Struct to store shell variables
typedef struct {
    char name[50];
    char value[50];
} Variable;

Variable variables[MAX_VARIABLES];
int variableCount = 0;

char history[HISTORY_SIZE][1024]; // Command history
int historyCount = 0;
int historyIndex = 0;

int lastStatus = 0;
char prompt[1024] = "hello: "; // Default prompt

volatile sig_atomic_t ctrl_c_pressed = 0;

// Signal handler for Control-C
void sigint_handler(int sig) {
    ctrl_c_pressed = 1;
}

// Set a shell variable
void set_variable(char *name, char *value) {
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strcpy(variables[i].value, value);
            return;
        }
    }
    strcpy(variables[variableCount].name, name);
    strcpy(variables[variableCount].value, value);
    variableCount++;
}

// Get the value of a shell variable
char* get_variable(char *name) {
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

// Substitute variables in the command
void substitute_variables(char *command) {
    char buffer[1024];
    char *start = command;
    char *pos;
    buffer[0] = '\0';

    while ((pos = strstr(start, "$")) != NULL) {
        strncat(buffer, start, pos - start);
        pos++;
        char varName[50];
        int varIndex = 0;
        while (isalnum(*pos) || *pos == '_') {
            varName[varIndex++] = *pos;
            pos++;
        }
        varName[varIndex] = '\0';
        char *varValue = get_variable(varName);
        if (varValue) {
            strcat(buffer, varValue);
        } else {
            strcat(buffer, "$");
            strcat(buffer, varName);
        }
        start = pos;
    }
    strcat(buffer, start);
    strcpy(command, buffer);
}

// Add a command to the history
void add_to_history(const char *command) {
    if (historyCount < HISTORY_SIZE) {
        strcpy(history[historyCount++], command);
    } else {
        // Shift history to make room for the new command
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i - 1], history[i]);
        }
        strcpy(history[HISTORY_SIZE - 1], command);
    }
    historyIndex = historyCount;
}

// Get the previous command from history
void get_previous_command(char *command) {
    if (historyIndex > 0) {
        historyIndex--;
        strcpy(command, history[historyIndex]);
    }
}

// Get the next command from history
void get_next_command(char *command) {
    if (historyIndex < historyCount - 1) {
        historyIndex++;
        strcpy(command, history[historyIndex]);
    } else {
        command[0] = '\0'; // Clear command if no next command
    }
}

// Enable raw mode for terminal input
void enable_raw_mode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);

    raw.c_lflag &= ~(ECHO | ICANON | ISIG); // Disable echo, canonical mode, and signals
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Disable raw mode for terminal input
void disable_raw_mode() {
    struct termios term;

    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ECHO | ICANON | ISIG); // Enable echo, canonical mode, and signals
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

// Handle the read command
void handle_read_command(char *variable) {
    char input[1024];
    printf("%s: ", variable);
    fgets(input, 1024, stdin);
    input[strlen(input) - 1] = '\0'; // Remove newline
    set_variable(variable, input);
}

// Clear the current line in the terminal
void clear_line() {
    printf("\33[2K\r"); // Clear the entire line and move cursor to the beginning
    fflush(stdout);
}

// Replace the current line with a new prompt and command
void replace_line(const char *prompt, char *command) {
    clear_line();
    printf("%s%s", prompt, command);
    fflush(stdout);
}

// Execute a single command
int execute_command(char *command);

// Read a line of input from the user, handling special keys
void read_line(char *buffer, const char *prompt) {
    int i = 0;
    int ch;

    printf("%s", prompt);
    fflush(stdout);

    while ((ch = getchar()) != '\n') {
        if (ch == 127) { // Handle backspace
            if (i > 0) {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == '\033') { // If the first value is an escape sequence
            getchar(); // Skip the '['
            switch (getchar()) { // The real value
                case 'A': // Up arrow
                    get_previous_command(buffer);
                    replace_line(prompt, buffer);
                    i = strlen(buffer);
                    continue;
                case 'B': // Down arrow
                    get_next_command(buffer);
                    replace_line(prompt, buffer);
                    i = strlen(buffer);
                    continue;
            }
        } else if (ch == 3) { // Handle Control-C
            ctrl_c_pressed = 1;
            return;
        } else {
            buffer[i++] = ch;
            putchar(ch);
            fflush(stdout);
        }
    }
    buffer[i] = '\0';
    putchar('\n');
}

// Handle if/then/else statements
void execute_if_else() {
    char condition[4096] = ""; // Buffer to store all if conditions
    char then_block[4096] = "";
    char else_block[4096] = "";
    char line[1024];
    int in_then_block = 0;
    int in_else_block = 0;

    // Read the entire if/then/else block until fi
    while (1) {
        read_line(line, in_then_block ? "then> " : (in_else_block ? "else> " : "if> "));
        if (ctrl_c_pressed) {
            ctrl_c_pressed = 0;
            printf("\n"); // Add line break before exit
            return;
        }

        if (strcmp(line, "then") == 0) {
            in_then_block = 1;
            continue;
        } else if (strcmp(line, "else") == 0) {
            in_else_block = 1;
            in_then_block = 0;
            continue;
        } else if (strcmp(line, "fi") == 0) {
            break;
        }

        if (in_then_block) {
            strcat(then_block, line);
            strcat(then_block, "\n");
        } else if (in_else_block) {
            strcat(else_block, line);
            strcat(else_block, "\n");
        } else {
            strcat(condition, line);
            strcat(condition, "\n");
        }
    }

    // Execute the combined if condition
    int condition_met = (execute_command(condition) == 0);

    // Execute the then block if condition met
    if (condition_met) {
        char *cmd = strtok(then_block, "\n");
        while (cmd != NULL) {
            execute_command(cmd);
            cmd = strtok(NULL, "\n");
        }
    } else {
        // Execute the else block if condition not met
        char *cmd = strtok(else_block, "\n");
        while (cmd != NULL) {
            execute_command(cmd);
            cmd = strtok(NULL, "\n");
        }
    }
}

// Execute a command, handling built-in commands and piping
int execute_command(char *command) {
    char *token;
    char *outfile;
    int i, fd, amper, redirect, retid, status, redirect_stderr, append;
    char **argv;
    
    argv = (char **)malloc(10 * sizeof(char *));
    
    // Tokenize command by pipes
    char *commands[10];
    int numCommands = 0;
    token = strtok(command, "|");
    while (token != NULL) {
        commands[numCommands++] = token;
        token = strtok(NULL, "|");
    }

    int pipefd[2], prevfd = -1;

    for (int cmdIndex = 0; cmdIndex < numCommands; cmdIndex++) {
        char *commandStr = commands[cmdIndex];
        // Trim leading and trailing whitespace
        while (isspace(*commandStr)) commandStr++;
        char *end = commandStr + strlen(commandStr) - 1;
        while (end > commandStr && isspace(*end)) end--;
        end[1] = '\0';

        // Parse the command into argv
        i = 0;
        token = strtok(commandStr, " ");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        argv[i] = NULL;

        // Handle variable assignment
        if (cmdIndex == 0 && argv[0] != NULL && strchr(argv[0], '$') == argv[0] && argv[1] != NULL && strcmp(argv[1], "=") == 0 && argv[2] != NULL) {
            set_variable(argv[0] + 1, argv[2]);
            free(argv);
            return 0;
        }

        // Built-in commands
        if (cmdIndex == 0) {
            if (argv[0] != NULL && strcmp(argv[0], "prompt") == 0 && strcmp(argv[1], "=") == 0 && argv[2] != NULL) {
                strcpy(prompt, argv[2]);
                strcat(prompt, ": ");
                free(argv);
                return 0;
            }
            if (argv[0] != NULL && strcmp(argv[0], "echo") == 0) {
                if (argv[1] != NULL && strcmp(argv[1], "$?") == 0) {
                    printf("%d\n", lastStatus);
                } else {
                    for (int j = 1; argv[j] != NULL; j++) {
                        if (argv[j][0] == '$') {
                            char *varValue = get_variable(argv[j] + 1);
                            if (varValue) {
                                printf("%s ", varValue);
                            } else {
                                printf("%s ", argv[j]);
                            }
                        } else {
                            printf("%s ", argv[j]);
                        }
                    }
                    printf("\n");
                }
                free(argv);
                return 0;
            }
            if (strcmp(argv[0], "cd") == 0) {
                if (argv[1] == NULL || chdir(argv[1]) != 0) {
                    perror("cd failed");
                }
                free(argv);
                return 0;
            }
            if ((strcmp(argv[0], "quit") == 0) || (strcmp(argv[0], "exit") == 0)) {
                printf("Exiting shell.\n");
                disable_raw_mode(); // Disable raw mode before exiting
                free(argv);
                exit(0);
            }
            if (strcmp(argv[0], "read") == 0) {
                if (argv[1] != NULL) {
                    disable_raw_mode(); // Disable raw mode for normal input handling
                    handle_read_command(argv[1]);
                    enable_raw_mode(); // Re-enable raw mode after reading input
                } else {
                    printf("Usage: read <variable>\n");
                }
                free(argv);
                return 0;
            }
            if (strcmp(argv[0], "if") == 0) {
                execute_if_else(); // Handle if/then/else
                free(argv);
                return 0;
            }
        }

        // Reset control variables for each command
        amper = redirect = redirect_stderr = append = 0;
        outfile = NULL;

        // Check for & (background), > (redirect stdout), 2> (redirect stderr), and >> (append)
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

        if (cmdIndex < numCommands - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(1);
            }
        } else {
            pipefd[0] = pipefd[1] = -1; // No pipe for the last command
        }

        if (fork() == 0) {
            // Child process

            if (prevfd != -1) {
                dup2(prevfd, 0);
                close(prevfd);
            }

            if (pipefd[1] != -1) {
                close(pipefd[0]);
                dup2(pipefd[1], 1);
                close(pipefd[1]);
            }

            // Handle redirection of stdout
            if (redirect) {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }

            // Handle redirection of stderr
            if (redirect_stderr) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                close(STDERR_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            // Handle appending to file
            if (append) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(argv[0], argv);
            // If execvp fails, print an error message directly to stderr
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(127); // Exit with 127 if exec fails
        }

        // Parent process
        if (prevfd != -1) {
            close(prevfd);
        }
        if (pipefd[1] != -1) {
            close(pipefd[1]);
        }
        prevfd = pipefd[0];

        if (!amper && cmdIndex == numCommands - 1) {
            retid = wait(&status);
            if (WIFEXITED(status)) {
                lastStatus = WEXITSTATUS(status); // Capture the exit status of the command
            } else {
                lastStatus = 1; // Non-zero status for abnormal exit
            }
        }
    }

    // Free dynamically allocated memory
    free(argv);
    return lastStatus;
}

int main() {
    char command[1024], lastCommand[1024] = "";

    signal(SIGINT, sigint_handler);  // Handle Control-C

    enable_raw_mode(); // Enable raw mode for arrow key detection

    while (1) {
        printf("%s", prompt); // Use the current prompt
        fflush(stdout); // Ensure the prompt is displayed immediately

        int i = 0;
        int ch;
        while ((ch = getchar()) != '\n') {
            if (ch == 127) { // Handle backspace
                if (i > 0) {
                    i--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (ch == '\033') { // If the first value is an escape sequence
                getchar(); // Skip the '['
                switch (getchar()) { // The real value
                    case 'A': // Up arrow
                        get_previous_command(command);
                        replace_line(prompt, command);
                        i = strlen(command);
                        continue;
                    case 'B': // Down arrow
                        get_next_command(command);
                        replace_line(prompt, command);
                        i = strlen(command);
                        continue;
                }
            } else if (ch == 3) { // Handle Control-C
                ctrl_c_pressed = 1;
                break;
            } else {
                command[i++] = ch;
                putchar(ch);
                fflush(stdout);
            }
        }
        command[i] = '\0';
        putchar('\n'); // Ensure command input ends on a new line

        if (ctrl_c_pressed) {
            printf("You typed Control-C!\n");
            ctrl_c_pressed = 0;
            continue; // Return to prompt
        }

        if (strlen(command) == 0) {
            continue;
        }

        add_to_history(command);

        substitute_variables(command);

        // Store the command before parsing for !! execution
        if (strcmp(command, "!!") != 0) {
            strcpy(lastCommand, command); // Update last command
        }

        // Handle !! command
        if (strcmp(command, "!!") == 0) {
            if (strlen(lastCommand) == 0) {
                printf("No commands in history.\n");
                continue;
            }
            printf("%s\n", lastCommand);
            strcpy(command, lastCommand);
        }

        // Disable raw mode if command is a cat redirect
        if (strstr(command, "cat >") == command) {
            disable_raw_mode();
        }

        execute_command(command);

        // Re-enable raw mode
        enable_raw_mode();
    }

    return 0;
}
