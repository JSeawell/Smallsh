# Smallsh
This program runs a custom shell script, similar to bash. Smallsh includes a command line prompt, input and output redirection, foreground and background processes, and three built-in commands: `cd`, `status`, and `exit`. All other commands are handled by an `exec()` function. Smallsh also handles comments.

## How to compile and run:
> Please compile, run, & test my program3: smallsh code using the following instructions:
> 1. Put `smallsh.c` into a directory  
> 2. While in that same directory, run the command:  
> `gcc -g smallsh.c -o smallsh`
> 3. Now that smallsh is an executable file, run it using the command:  
> `./smallsh`

## What I learned:
> 1. Processes (parents, children, zombies)  
> 2. `Fork()` and `exec()`
> 3. Signals  
> 4. Environment variables  
> 5. File inheritance, input/output redirection
