#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define LSH_RL_BUFSIZE 512
char *afsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c; // Store characters as int, to accomodate for EOF and suchlike

    if (!buffer) {
        fprintf(stderr, "afsh: allocation error!\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character!
        c = getchar();

        // If the char is EOF, replace it with null and return
        if(c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "afsh: re-allocation error!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **afsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(sizeof(char) * bufsize);
    char *token;

    if (!tokens) {
        fprintf(stderr, "afsh: allocation error!\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, sizeof(char*) * bufsize);
            if (!tokens) {
                fprintf(stderr, "afsh: re-allocation error!\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int afsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "afsh child process error!\n");
        }
        status = 1;
    } else if (pid < 0) {
        // error forking
        fprintf(stderr, "afsh forking error!\n");
        status = 2;
    } else {
        // parent process
        // pid_t wpid; //unused as of now
        do {
            //wpid = waitpid(pid, &status, WUNTRACED);
            waitpid(pid, &status, WUNTRACED);
            //fprintf(stderr, "afsh:  child pid: %d\n", status);
        } while ( !WIFEXITED(status) && !WIFSIGNALED(status) );
        //status = 0;
    }

    return status;
}

// Function Declarations for builtin shell commands:

int afsh_cd(char **args);
int afsh_help(char **args);
int afsh_exit(char **args);

// List of builtin commands, followed by their corresponding functions.

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &afsh_cd,
    &afsh_help,
    &afsh_exit
};

int afsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

// Builtin function implementations.

// Changes working directory
int afsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "afsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("afsh");
        }
    }
    return 1;
}

// Help
int afsh_help(char **args)
{
    int i;
    printf("afsh\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < afsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

// Exit
int afsh_exit(char **args)
{
    return 0;
}

// Launch or execute
int afsh_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 0; // I don't think that should be a failure
    }

    for (i = 0; i < afsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return afsh_launch(args);
}

int afsh_loop(void)
{
    char *line;
    char **args;
    int status = EXIT_SUCCESS;

    do {
        printf("{%d} > ", status);
        line = afsh_read_line();
        args = afsh_split_line(line);
        status = afsh_execute(args);

        free(line);
        free(args);
    } while (1);
    return status;
}

int main(int argc, char **argv)
{
    int status = EXIT_SUCCESS;
    // The shell's command loop
    status = afsh_loop();
    return status;
}

