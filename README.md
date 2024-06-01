# myshell

## Description

`myshell` is a custom shell program with features such as command execution, background execution, output redirection, shell variables, command history, and more.

## Features

1. Execute commands with arguments.
2. Background execution using `&`.
3. Output redirection using `>`, `2>`, and `>>`.
4. Change shell prompt using `prompt = myprompt`.
5. Built-in `echo` command.
6. Print the status of the last executed command using `echo $?`.
7. Change the current directory using `cd`.
8. Repeat the last command using `!!`.
9. Exit the shell using `quit`.
10. Handle `Control-C` with a custom message.
11. Pipe multiple commands using `|`.
12. Define and use shell variables.
13. Read input from the user using `read`.
14. Maintain a history of the last 20 commands.

## Build and Run

To build and run the shell, use the following commands:

```sh
make
./myshell
```

Usage
Type commands as you would in a normal shell. Here are some examples:

- Execute a command: ls -l
- Run a command in the background: ls -l &
- Redirect output to a file: ls -l > output.txt
- Redirect errors to a file: ls -l non_existent_file 2> error.txt
- Append output to a file: ls -l >> output.txt
- Change prompt: prompt = myprompt
- Use echo to print arguments: echo Hello, World!
- Print the last command's status: echo $?
- Change directory: cd /path/to/directory
- Repeat last command: !!
- Exit the shell: quit
- Handle Control-C: Press Control-C
- Pipe commands: ls | grep myfile
- Define and use a variable: person = John, echo $person
- Read input: read name, then enter a value
