# ECS 150 FQ 2024 Project 1: Simple Shell Report
# By Ethan Park and Jason Yoo

## Summary

This simple shell program can mimic the functions of a shell at a basic level. 
Specifically, this program can perform shell commands such as `cd`, support output 
redirection and appending, and piping.

## Implementation

The implementation of this program follows these steps:

1. Repeatedly ask and scan the input given from the user and detect what the string 
has.
2. While scanning, if the code dectects any errors with the command, we output error 
message and skip to the next iteration.
3. Depending on if the string contains specific characters that denote a certain 
command, we parse the string and run the commands accordingly.
4. Once the code has successfully parsed, we also retrieved information on the 
commands and send the parsed information into specific built in commands the input 
called for.
5. When the code has executed the necessary commands and reached the end 
of the loop, we print out a 'completed' message that informs the user if their
input executed successfully or not.

## Getting User input

At the beginning of the execution, we initialize the proper variables and enter a 
while loop that will allow the program to constantly input and execute commands. 
Using the `fgets()` function, we can retrieve a command line string that requires 
parsing. `fget()` is the best choice in getting user input as it allows us to 
specify number of characters to read and the specificed number is set in 
`#define CMDLINE_MAX`. After receiving the input, we remove `\n` character and 
append a null character `\O` at the end of the string which is valueable in parsing.

## Character scanning

Before we start parsing the command line string, we first detect if the string 
contains the meta character `|` which denotes that the input requires piping. This 
method is useful as it allows us to send our command line into a specific parsing 
function designed to parse the command line correctly. We also remove any leading 
spaces from the command to make detecting commands easier.

## Parsing

For parsing, the important C string functions that we used to successfully parse the 
command line were `strtok`, `strstr`, `strcmp`, `strncmp`, and `strcpy`. A major 
issue in the command line was that we are given one whole string, however, `strtok` 
allowed us to split the whitespaces between characters to sort them into a string of 
arguments.

We implemented our parsing algorithm into the `parsingArguments()` function 
that is called after receiving the input. This function parses the command line 
thorugh "if" statements and string function to detect if it's an output redirection 
command or if it's a string of arguments. 

For us, we felt using an array was the best for us as a simple array allowed us to 
have an easier time accessing it and connecting with other functions. Also the command 
would always be at `args[0]` so we did not run into too much trouble when executing 
the argument. 

Some limitations with that we encountered with our parsing is that if characters that 
denote certain actions like output redirection `>` or piping `|` do not have 
whitespaces between them can create issues since our parses handled whitespaces. To 
fix this we implemented `strcmp` to handle issues where there's a whitespace between 
`>` and strstr to handle issues where there are no whitespaces between the `>`.

## Error Checking

While parsing, we also set up parts in the parsing function that can handle error
management. For example, the `parsingArguments()` function, we added a checker to 
count the amount of arguments and if the arguments exceeded a certain value,
we would inform the user about an error with stderr and skip to the next iteration
with a return value of 1(signifies error). 

We also set up error checks with the syscall functions and if an error is detected 
we output a message using stderr and skip to the next iteration with a message that 
shows that the command was completed with a 1.

## Builtin Commands

In this project, the following builtin commands we implemented are the following: 
`exit`, `cd`, `pwd`, `>` (output redirection), `>>`(appending), `|`(piping), `sls`. 
Using execvp(), we were also able to execute other commands such as `date`. The way 
our `cd` and `pwd` algorithm works is that it detects if the command line contains t
the characters `cd` or `pwd` and executes using directory syscall functions. 

We start a child process using the syscall function `fork()` and we ensure the child 
process is completed first through the `waitpid()` function. We send the arguments 
into a function called `executeCommand()`. The code first detects if the command is 
an output redirection command by checking if the filename variable is `NULL` and 
the output is appending to file thorugh a flag that is checked from the parsing stage.

## Piping

Once it detects piping `|` in the command, we determined how many pipes are needed 
and separated each command into an array using `pipeParsing()`. While parsing each 
command, we made sure that number of commands equal to number of pipes + 1. Then, 
we create pipes using `pipe()` function for the right amount of times. 

Using for loop and `parsingArguments()`, we parsed each command that were separated 
by `pipeParsing()` and checked if there was misloacted output redirection. 

Using for loop and `fork()`, we set each child's `STDIN_FILENO` and 
`STDOUT_FILENO` to each pipes using `dup2()` accordingly and close all unused file
descriptors using `closeFds()` which takes an array of file descriptors and its size.
Then using the parsed arguments using `executeCommand()` that utilizes `execvp()` to
execute command and return their exit values.

Finally, using for loop, we wait for each child to exit their process with `waitpid()`
and save their return values

## Testing

We tested our code using the tester.sh file provided by the professor and we fixed
any problems according to the failed test cases. Once we resolved the cases from the
tester.sh, we tested the code from the project1.html file for any other possible 
errors in our code. For uncertain errors or cases, we compared the output that 
sshell_ref outputs. Most of the errors we encountered were parsing errors or errors
using the syscalls functions. 

## References

Strtok - (https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/)
Strcmp - (https://www.tutorialspoint.com/c_standard_library/c_function_strcmp.html)
Strstr - (https://www.geeksforgeeks.org/strstr-in-ccpp/)
Linux Man pages, for functions related to syscalls - (https://linux.die.net/man/)
ECS 150 slides