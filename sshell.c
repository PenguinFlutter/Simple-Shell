#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX   512
#define PIPE_MAX      5
#define FD_RW         2
#define MAX_ARGS      16
#define TOKEN_LEN_MAX 32

void removeLeadingSpaces(char *str) {
    int i, j;

    for (i = 0; str[i] != '\0' && isspace((unsigned char)str[i]); i++) {
        // Skipping leading whitespace
    }

    // Shift the string to fill the leading whitespace
    for (j = 0; str[i] != '\0'; i++, j++) {
        str[j] = str[i];
    }

    // Add null character to the end of the modified string
    str[j] = '\0';
}

int count_pipe(const char *str) {
    int count = 0;
    
    while (*str) {
        if (*str == '|') {
            count++;
        }
        str++;
    }
    
    return count;
}

int pipeParsing(char *cmd, char **args, int num_pipes) {
    char *token; 
    char orig_command[CMDLINE_MAX];
    strcpy(orig_command, cmd);

    // Separate each command using '|' as delimiter
    token = strtok(cmd, "|");

    int i = 0;
    while (token != NULL && i < PIPE_MAX - 1) {
        args[i++] = token;
        token = strtok(NULL, "|");
    }

    // Check if we have right number of arguments
    // by comparing the last command (random if not assigned)
    // with the original command
    if (!strstr(orig_command, args[num_pipes])) {
        fprintf(stderr, "Error: missing command\n");
        return 1;
    }
    return 0;
}

void closeFds(int (*fd)[FD_RW], int size) {
    for (int i = 0; i < size; ++i){
        close(fd[i][0]);
        close(fd[i][1]);
    }
}

char *parsingArguments(char cmd[], char *args[CMDLINE_MAX], int *next_iter, int *app_mode){
    char *token; 
    char *filename = NULL;

    // Separate each argument using ' ' as delimiter
    token = strtok(cmd, " ");

    int i = 0;
    while (token != NULL && i < CMDLINE_MAX - 1) {
        if (i >= MAX_ARGS) {
            fprintf(stderr, "Error: too many process arguments\n");
            *next_iter = 1;
            break;
        }
        
        if (strcmp(token, ">") == 0) { // Handles whitespaces between >
            token = strtok(NULL, " ");
            filename = token;
            if (filename == NULL) {
                fprintf(stderr, "Error: no output file\n");
                *next_iter = 1;
                break;
            } 
            break;
        } else if (strcmp(token, ">>") == 0) { // Handles whitespaces bewteen >>
            *app_mode = 1;
            token = strtok(NULL, " ");
            filename = token;
            break;
        } else if (strstr(token, ">>") != NULL) { // Handles no whitespaces between >>
            *app_mode = 1;
            args[i++] = strtok(token, ">>");
            filename = strtok(NULL, ">>");
            break;
        } else if (strstr(token, ">") != NULL) { // Handles no whitespaces between >
            args[i++] = strtok(token, ">");
            filename = strtok(NULL, ">");
            break;
        } else { // Save the argument and keep searching
            args[i++] = token;
            token = strtok(NULL, " ");
        }
    }

    args[i] = NULL;
    return filename;
}

// Only called in the child process
void executeCommand(char *args[], char *filename, int app_mode) { 
    int fd;
    if (filename != NULL && app_mode == 0) { // Output redirection, truncating
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    } else if (filename != NULL && app_mode == 1) { // Output redirection, appending
        fd = open(filename, O_WRONLY | O_APPEND, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO); 
        close(fd);  
    }

    // Execute given command with arguments, if failed 
    // Print out Error to stderr and terminate child process
    execvp(args[0], args); 
    fprintf(stderr, "Error: command not found\n");
    exit(1);
}

void pipeStatusReport(char *orig_command, int *retvals, int num_pipes) {
    fprintf(stderr, "+ completed '%s'", orig_command);
    for (int i = 0; i < num_pipes+1; ++i){
        fprintf(stderr, "[%d]", retvals[i]);
    }
    fprintf(stderr, "\n");
}

int main(void) {
    char cmd[CMDLINE_MAX];
    char orig_command[CMDLINE_MAX];
    char *nl;
    int retval;
    int isPipe;
    int end = 0;

    while (!end) {
        char *filename = NULL;
        int app_mode = 0;
        int next_iter = 0;

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';
        isPipe = strstr(cmd, "|") != NULL;

        /* Save original command for status printout */
        strcpy(orig_command, cmd);

        /* Remove leading space to use strncmp for builtin commands */
        removeLeadingSpaces(cmd);

        if (cmd[0]=='\0') {
            fprintf(stderr, "Error: missing command\n");
            continue;
        }

        /* Builtin command */
        if (!isPipe) { // no piping
            if (!strncmp(cmd, "exit", 4)) {
                fprintf(stderr, "Bye...\n");
                retval = 0;
                fprintf(stderr, "+ completed '%s' [%d]\n", orig_command, retval);
                break;
            } else if (!strncmp(cmd, "cd ", 3)) { // cd command
                char *arg = cmd + 3;
                
                if (chdir(arg) != 0)  { 
                    fprintf(stderr, "Error: cannot cd into directory\n");
                    retval = 1;
                } else {
                    retval = 0;
                }

            } else if (!strncmp(cmd, "pwd", 3)) { // pwd command
                char cwd[CMDLINE_MAX];

                if (getcwd(cwd, sizeof(cwd)) != NULL) {  
                    fprintf(stdout, "%s\n", cwd);
                    retval = 0;
                } else { 
                    fprintf(stderr, "Error; cannot pwd directory\n");
                    retval = 1;
                }

            } else if (strcmp(cmd, "sls") == 0) { // sls command
                char cwd[CMDLINE_MAX];
                DIR *dir;
                struct dirent *dp;
                struct stat sb;

                dir = opendir(".");
                getcwd(cwd, sizeof(cwd));

                while ((dp = readdir(dir)) != NULL) {
                    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) { // Skip directories
                        continue;  
                    }
                    stat(dp->d_name, &sb); // Obtain the information about the files
                    printf("%s (%lld bytes)\n", dp->d_name, (long long) sb.st_size);
                }
                closedir(dir); // Close directory
                retval = 0;

            } else { // Other commands
                char *args[CMDLINE_MAX];
                filename = parsingArguments(cmd, args, &next_iter, &app_mode);
                retval = next_iter;

                // Error was found while parsing, go to next iteration
                if (next_iter == 1) {
                    continue; 
                }

                pid_t pid = fork(); 
                if (pid == 0) { // Child proces
                    executeCommand(args, filename, app_mode);
                } else { // Parent process
                    int status;
                    waitpid(pid, &status, 0); 
                    retval = WEXITSTATUS(status);
                }
            }
            fprintf(stderr, "+ completed '%s' [%d]\n", orig_command, retval);

        } else { // Piping involved
            int num_pipes = count_pipe(cmd);
            char *pipe_args[PIPE_MAX];
            pid_t pids[num_pipes+1];
            int retvals[num_pipes+1];
            int fd[num_pipes][FD_RW];
            char *args[num_pipes+1][CMDLINE_MAX];
            char *filenames[num_pipes+1];
            
            // If we are missing commands pipeParsing returns 1
            // Continue to next iteration
            if (pipeParsing(cmd, pipe_args, num_pipes))
                continue;
            
            // Open pipes 
            for (int i = 0; i < num_pipes; ++i){
                pipe(fd[i]);
            }

            // Parse each command 
            for (int i = 0; (i < num_pipes+1) & (next_iter == 0); ++i) {
                filenames[i] = parsingArguments(pipe_args[i], args[i], &next_iter, &app_mode);

                if (i != num_pipes && filenames[i] != NULL) {
                    fprintf(stderr, "Error: mislocated output redirection\n");
                    next_iter = 1;
                }                   
            }

            // If error found while parsing
            // continue to next iteration
            if (next_iter == 1)
                continue;

            for (int i = 0; i < num_pipes+1; ++i){
                if (!(pids[i] = fork())) {
                    if (i!=0) 
                        dup2(fd[i-1][0], STDIN_FILENO);

                    if (i!=num_pipes)
                        dup2(fd[i][1], STDOUT_FILENO); 

                    closeFds(fd, num_pipes);
                    executeCommand(args[i], filenames[i], app_mode);
                }
            }
            
            closeFds(fd, num_pipes);

            int status;
            for (int i = 0; i < num_pipes+1; ++i){
                waitpid(pids[i], &status, 0);
                retvals[i] = WEXITSTATUS(status);
            }
            pipeStatusReport(orig_command, retvals, num_pipes);
        }
    }

    return EXIT_SUCCESS;
}