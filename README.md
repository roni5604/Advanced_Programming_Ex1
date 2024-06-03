
# myshell

## Description

`myshell` is a custom shell program that supports various advanced features such as command execution, background execution, output redirection, shell variables, command history, and control flow with `if/then/else` statements.

## Features

1. **Execute Commands with Arguments**:
   - **Implementation**: This is the basic functionality implemented in the `execute_command` function. It uses `fork` and `execvp` to execute commands.
   - **Location**: `execute_command` function.

2. **Background Execution**:
   - **Implementation**: Detects `&` at the end of commands to run them in the background.
   - **Location**: `execute_command` function.

3. **Output Redirection**:
   - **Implementation**: Handles `>`, `2>`, and `>>` for redirection of standard output and error.
   - **Location**: `execute_command` function.

4. **Change Shell Prompt**:
   - **Implementation**: Allows changing the prompt using `prompt = newprompt`.
   - **Location**: Built-in command handling in `execute_command`.

5. **Built-in `echo` Command**:
   - **Implementation**: Implements an `echo` command to print arguments.
   - **Location**: Built-in command handling in `execute_command`.

6. **Print Status of Last Command**:
   - **Implementation**: Uses `echo $?` to print the status of the last executed command.
   - **Location**: Built-in `echo` command in `execute_command`.

7. **Change Directory**:
   - **Implementation**: Implements the `cd` command to change the current directory.
   - **Location**: Built-in command handling in `execute_command`.

8. **Repeat Last Command**:
   - **Implementation**: Uses `!!` to repeat the last command.
   - **Location**: Main loop and `execute_command` function.

9. **Exit the Shell**:
   - **Implementation**: Allows exiting the shell using `quit` or `exit`.
   - **Location**: Built-in command handling in `execute_command`.

10. **Handle Control-C**:
    - **Implementation**: Captures `Control-C` to print a custom message and prevent shell exit.
    - **Location**: `sigint_handler` and main loop.

11. **Pipe Multiple Commands**:
    - **Implementation**: Uses `|` to pipe commands.
    - **Location**: `execute_command` function.

12. **Shell Variables**:
    - **Implementation**: Allows defining and using variables. Example: `person = John`, `echo $person`.
    - **Location**: `set_variable`, `get_variable`, and `substitute_variables` functions.

13. **Read Input**:
    - **Implementation**: Implements the `read` command to read user input.
    - **Location**: Built-in command handling in `execute_command`.

14. **Command History**:
    - **Implementation**: Maintains a history of the last 100 commands. Uses up and down arrows to navigate.
    - **Location**: `add_to_history`, `get_previous_command`, `get_next_command`, and raw mode handling functions.

15. **Control Flow with `if/then/else`**:
    - **Implementation**: Supports flow control similar to shell scripting.
    - **Location**: `execute_if_else` function.

## Build and Run

To build and run the shell, use the following commands:

```sh
make
./myshell
```

## Usage

Type commands as you would in a normal shell. Here are some examples:

- **Execute a command**: `ls -l`
- **Run a command in the background**: `ls -l &`
- **Redirect output to a file**: `ls -l > output.txt`
- **Redirect errors to a file**: `ls -l non_existent_file 2> error.txt`
- **Append output to a file**: `ls -l >> output.txt`
- **Change prompt**: `prompt = myprompt`
- **Use echo to print arguments**: `echo Hello, World!`
- **Print the last command's status**: `echo $?`
- **Change directory**: `cd /path/to/directory`
- **Repeat last command**: `!!`
- **Exit the shell**: `quit`
- **Handle Control-C**: Press `Control-C`
- **Pipe commands**: `ls | grep myfile`
- **Define and use a variable**: `person = John`, `echo $person`
- **Read input**: `read name`, then enter a value
- **If/Then/Else statements**:
  ```sh
  if date | grep Fri
  then
    echo "Shabbat Shalom"
  else
    echo "Hard way to go"
  fi
  ```

## Authors

`myshell` was developed by Roni Michaeli & Elor Israeli.
