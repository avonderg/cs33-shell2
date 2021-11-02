#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// function declarations
void parse_helper(char buffer[1024], char *tokens[512], char *argv[512],
                  char r[20]);
int parse(char buffer[1024], char *tokens[512], char *argv[512],
          char *w_sym[512], const char **input_file, const char **output_file,
          int *output_flags, char **path);
int built_in(char *argv[512], char **path);
int cd(char *dir);
int ln(char *src, char *dest);
int rm(char *file);
int file_redirect(const char **input_file, const char **output_file,
                  int *output_flags);
int set_path(char *tokens[512], char **path);

/**
 * Main function
 *
 * Returns:
 *  - 0 if EOF is reached, 1 if there is an error
 **/
int main() {
    // repl (read eval print loop)
    while (1) {
#ifdef PROMPT
        if (printf("33sh> ") < 0) {
            fprintf(stderr, "error: unable to write");
            return 1;
        }
        if (fflush(stdout) < 0) {
            perror("fflush");
            return 1;
        }
#endif

        // initializing
        char buf[1024];
        memset(buf, '\0', 1024);  // 512 * size of char pointer
        int fd = STDIN_FILENO;
        size_t count = 1024;
        ssize_t to_read;
        // reading system call to get user input
        to_read = read(fd, buf, count);
        // error-checking
        // check if its equal to \n or spaces
        // check argv array (case where there are no tokens)
        if (to_read == -1) {
            perror("error: read");
            return 1;
        } else if (to_read == 0) {  // restart program
            return 0;               // 0 means end of file
        }
        buf[to_read] =
            '\0';  // since the read function does not null-terminate the buffer
        char *tokens[512];
        char *argv[512];
        char *w_sym[512];
        memset(tokens, '\0', 4096);  // 512 * size of char pointer
        memset(argv, '\0', 4096);    // 512 * size of char pointer
        memset(w_sym, '\0', 4096);   // 512 * size of char pointer
        char *path = NULL;
        const char *input_file = NULL;
        const char *output_file = NULL;
        int *output_flags = NULL;  // flag is set to 2 if flag = O_APPEND, and 1
                                   // if flag = O_TRUNC
        int val;
        output_flags = &val;
        int parse_result = parse(buf, tokens, argv, w_sym, &input_file,
                                 &output_file, output_flags, &path);
        if (argv[0] == NULL) {
            continue;
        }
        if (parse_result == 0) {
            continue;
        }
        int built_ins = built_in(argv, &path);
        if (built_ins == 0) {
            pid_t pid;
            if ((pid = fork()) == 0) {  // enters child process
                int redirects =
                    file_redirect(&input_file, &output_file, output_flags);
                if (redirects == -1) {  // if an error has occured
                    return 1;
                }
                int exec = execv(path, argv);
                if (exec == -1) {
                    perror("execv");
                }
                perror("child process could not do execv");
            } else if (pid > 0) {  // enters wait mode
                wait(NULL);
            } else {  // if an error has ocurred
                perror("error calling function fork()");
                exit(0);
            }
        }
    }
    return 0;
}

/**
 * Helper function for parse(), parses an input buffer
 *
 * Parameters:
 * - buffer: input buffer
 * - tokens: tokens array to fill with tokenized values
 * - argv: a pointer to the first element in the command line
 *            arguments array
 * - r: an array of characters to remove from tokens
 *
 * **/
void parse_helper(char buffer[1024], char *tokens[512], char *argv[512],
                  char r[20]) {
    char *temp;  // temp string to hold values
    int n = 0;
    temp = strtok(buffer, r);  // tokenizes temp char, only returns first token
    while (temp !=
           NULL) {  // loop through the temp string in order to find all tokens
        tokens[n] = temp;  // value stored in tokens array
        temp = strtok(
            NULL,
            r);  // goes to next character in string that is not whitespace
        n++;
    }
    if (tokens[0] == NULL) {
        argv[0] = NULL;
    }
    char *first = strtok(buffer, r);  // gets first token (binary name)
    if (first == NULL) {              // base case
        argv[0] = NULL;
    } else {
        const char slash = '/';
        char *last = strrchr(
            first,
            slash);  // returns pointer to the last occurence of the slash
        if (last == NULL) {  // if there are no slashes in the first arg
            argv[0] = first;
        } else {
            last++;  // otherwise, goes to the part of the token that follows
                     // the last slash
            if (last == NULL) {
                argv[0] =
                    "";  // if there is no other arg, put in an empty string
            } else {
                argv[0] = last;  // otherwise, set first argument equal to the
                                 // following one
            }
        }
        int index = 1;
        while (tokens[index] != NULL) {   // looping through tokens string
            argv[index] = tokens[index];  // sets arguments equal to the
                                          // appropriate tokens
            index++;                      // increments index
        }
    }
}

/**
 * Parses and tokenizes an input buffer, and fills the argv array appropriately
 * depending on the existence of file redirection symbols.
 *
 * Parameters:
 * - buffer: input buffer
 * - tokens: tokens array to fill with tokenized values
 * - argv: a pointer to the first element in the command line
 *            arguments array
 * - w_sym: temp array to pass in to parse_helper, includes file redirects and
 * filenames
 * - input_file: a pointer to the input file (if it exists)
 * - ouptput_file: a pointer to the output file (if it exists)
 * - path: a pointer to the filepath, to pass in to execv()
 *
 * Returns:
 * - 0 if an error occured, 1 otherwise
 * **/
int parse(char buffer[1024], char *tokens[512], char *argv[512],
          char *w_sym[512], const char **input_file, const char **output_file,
          int *output_flags, char **path) {
    int i = 0;  // index for tokens
    int k = 0;  // index for argv array
    int flag1 = 0;
    int flag2 = 0;
    char r1[3] = {' ', '\t', '\n'};  // characters to tokenize
    parse_helper(buffer, tokens, w_sym, r1);
    while (tokens[i] != NULL) {  // looping through tokens array
        if (strcmp(tokens[i], "<") == 0) {
            // error check first
            flag1++;          // set flag to 1- meaning that it was found
            if (flag1 > 1) {  // if input redirect appeared 2x
                fprintf(stderr, "syntax error: multiple input files");
                return 0;
            }
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "No redirection file specified.");
                return 0;
            }
            if (strcmp(tokens[i + 1], ">") == 0) {
                fprintf(stderr, "No redirection file specified.");
                return 0;
            }
            if (strcmp(tokens[i + 1], ">>") == 0) {
                fprintf(stderr, "No redirection file specified.");
                return 0;
            }
            // after error checking is complete
            *input_file = tokens[i + 1];
            i += 2;
        } else if (strcmp(tokens[i], ">") == 0) {
            // error check first
            flag2++;            // set flag to 1- meaning that it was found
            *output_flags = 1;  // O_TRUNC
            if (flag2 > 1) {    // if output redirect appeared 2x
                fprintf(stderr, "syntax error: multiple output files");
                return 0;
            }
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "No redirection file specified.");
                return 0;
            }
            // after error checking is complete
            *output_file = tokens[i + 1];
            i += 2;
        } else if (strcmp(tokens[i], ">>") == 0) {
            // error check first
            flag2++;            // set flag to 1- meaning that it was found
            *output_flags = 2;  // O_APPEND
            if (flag2 > 1) {    // if output redirect appeared 2x
                fprintf(stderr, "syntax error: multiple output files");
                return 0;
            }
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "No redirection file specified.");
                return 0;
            }
            // after error checking is complete
            *output_file = tokens[i + 1];
            i += 2;
        } else {  // otherwise, then add in element to argv
            argv[k] = w_sym[i];
            k++;
            i++;
        }
    }
    if (flag1 != 1 && flag2 != 1) {
        *path = tokens[0];
    } else {
        set_path(tokens, path);
    }
    return 1;
}

/**
 * Sets the filepath to be passed in to execv() using the first element in the
 * tokenized array, provided it is not a file direct or a file.
 *
 * Parameters:
 * - tokens: tokens array to fill with tokenized values
 * - path: a pointer to the filepath, to pass in to execv()
 *
 * Returns:
 * - 0 if path set successfully, 1 otherwise
 * **/
int set_path(char *tokens[512], char **path) {
    int i = 0;
    while (tokens[i] != NULL) {
        if ((strcmp(tokens[i], "<") != 0) && (strcmp(tokens[i], ">") != 0) &&
            (strcmp(tokens[i], ">>") != 0)) {
            *path = tokens[i];
            return 0;
        } else {  // if the current index is a symbol
            i++;  // skip over an index (the file)
        }
        i++;
    }
    return 1;
}

/**
 * Handles built_in commands by calling the appropriate system calls
 *
 * Parameters:
 * - argv: a pointer to the first element in the command line
 *            arguments array
 * - path: a pointer to the filepath, to pass in to execv()
 *
 * Returns:
 * - -1 if an error occured, 1 if successful, and 0 if there was no command
 * foudn
 * **/
int built_in(char *argv[512], char **path) {
    if (strcmp(*path, "cd") == 0) {  // if the command is cd
        // char *dir = argv[1];
        // pass in elt after 'cd'
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: syntax error");
        } else if (chdir(argv[1]) == -1) {
            perror(argv[0]);
        }
        return 1;
    } else if (strcmp(*path, "ln") == 0) {  // if the command is ln
        if (argv[1] == NULL) {
            fprintf(stderr, "ln: syntax error");
        } else if (argv[2] == NULL) {
            fprintf(stderr, "ln: syntax error");
        } else if (link(argv[1], argv[2]) != 0) {  // error checking
            perror("link");
            return -1;
        }
        return 1;
    } else if (strcmp(*path, "rm") == 0) {  // if the command is rm
        if (argv[1] == NULL) {
            fprintf(stderr, "rm: syntax error");
        } else if (unlink(argv[1]) != 0) {
            perror("unlink");
            return -1;
        }
        return 1;
    } else if (strcmp(*path, "exit") == 0) {  // if the command is exit
        exit(0);
    }
    return 0;
}

/**
 * Handles file redirection by opening the appropriate files based on whether it
 * is an input file, an output file to append, or an output file to truncate.
 *
 * Parameters:
 * - input_file: a pointer to the input file (if it exists)
 * - ouptput_file: a pointer to the output file (if it exists)
 * - output_flags: an integer pointer representing which type of output file
 * redirection, if any, is being used. 1 indicates that it must truncate, and 2
 * indicates it must append
 *
 * Returns:
 * - -1 if an error occured, 0 otherwise
 * **/
int file_redirect(const char **input_file, const char **output_file,
                  int *output_flags) {
    if (*input_file != NULL) {  // if there is an input file
        int closed = close(STDIN_FILENO);
        if (closed != 0) {
            perror("error: close");
            return -1;
        }
        int open_descr = open(*input_file, O_RDONLY);  // open file to read
        if (open_descr == -1) {
            perror("error: open");
            return -1;
        }
    }
    if ((*output_file != NULL) &&
        (*output_flags == 1)) {  // if there is an output file to truncate
        int closed = close(STDOUT_FILENO);
        if (closed != 0) {
            perror("error: close");
            return -1;
        }
        int open_descr = open(*output_file, O_CREAT | O_WRONLY | O_TRUNC,
                              0644);  // open file to read
        if (open_descr == -1) {
            perror("error: open");
            return -1;
        }
    }
    if ((*output_file != NULL) &&
        (*output_flags == 2)) {  // if there is an output file to append
        int closed = close(STDOUT_FILENO);
        if (closed != 0) {
            perror("error: close");
            return -1;
        }
        int open_descr = open(*output_file, O_WRONLY | O_CREAT | O_APPEND,
                              0666);  // open file to read
        if (open_descr == -1) {
            perror("error: open");
            return -1;
        }
    }
    return 0;
}