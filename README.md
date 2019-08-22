# Smallsh
> CS-344 (Operating Systems I) Program  

## Description:
> This program runs a custom shell script, similar to bash. Smallsh includes a command line prompt, input and output redirection, foreground and background processes, and three built-in commands: `cd`, `status`, and `exit`. All other commands are handled by an `exec()` function. Smallsh also handles comments.

## How to compile and run:
> Please compile, run, & test my program3: smallsh code using the following instructions:
> 1. Put `smallsh.c` into a directory  
> 2. While in that same directory, run the command:  
> `gcc -g smallsh.c -o smallsh`
> 3. Now that smallsh is an executable file, run it using the command:  
> `./smallsh`

## Tech and/or concepts learned/used:
> - C language
> - Bash
> - Processes (parents, children, zombies)  
> - `Fork()` and `exec()`
> - Signals  
> - Environment variables  
> - File inheritance, input/output redirection
