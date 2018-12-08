#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
char *name = "mysh";
char **tokens;
int token_count;
int *opt;
char *cwd;
int status = 0;
char ***split;
int *count;

//--------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------
int tokenize(char *);
void eval();
void set_cwd();
void fun_name(int);
void fun_help();
void fun_status();
void fun_exit(int);
void fun_print(int);
void fun_echo(int);
void fun_pid();
void fun_ppid();
void fun_dir(int);
void fun_dirwhere();
void fun_dirmake();
void fun_dirremove();
void fun_dirlist(int);
void fun_linkhard();
void fun_linksoft();
void fun_linkread();
void fun_linklist();
void fun_unlink();
void fun_rename();
void fun_cpcat(int);
void fun_pipes(int);
void pipe_start(int[], int);
void pipe_middle(int[], int[], int);
void pipe_end(int[], int);
void fun_exec_front(int *);
void fun_exec_back(int *);
void handler(int);
int fun_exec_internal(int);

int main (int argc, char *argv[]) {
    //-------------------------------------------------------------------------------
    // Set SIGCHLD signal handler
    //-------------------------------------------------------------------------------
    if (signal(SIGCHLD, handler) < 0) {
        perror("signal");
    }
    //-------------------------------------------------------------------------------
    // 1. Non-interactive / script mode
    //-------------------------------------------------------------------------------
    if (isatty(0) == 0) {
        while (1) {
            fflush(stdout);
            // reading the line
            char *line = NULL;
            char c[1];
            int i = 0, n;
            while (1) {
                n = read(0, c, 1);
                if (n < 0) {
                    int e = errno;
                    perror("read");
                    exit(e);
                } else {
                    // new character
                    line = (char *) realloc(line, (i+1) * sizeof(char));
                    line[i++] = c[0];
                    // end of the line
                    if (c[0] == '\n') {
                        break;
                    }
                }
            }
            // check if we reached the end of the file
            if (n == 0) {
                break;
            }
            // a command was entered
            if (strlen(line) > 1) {
                // get command data and execute it if it satifies conditions
                if (tokenize(line)) {
                    eval();
                }
            }
        }
    //-------------------------------------------------------------------------------
    // 2. Interactive mode (manual input of commands)
    //-------------------------------------------------------------------------------
    } else {
        while (1) {
            // show prompt
            printf("%s> ", name);
            fflush(stdout);
            // read the line
            char *line = NULL;
            char c[1];
            int i = 0, n;
            while (1) {
                n = read(0, c, 1);
                // error
                if (n < 0) {
                    int e = errno;
                    perror("read");
                    exit(e);
                } else {
                    // new character
                    line = (char *) realloc(line, (i+1) * sizeof(char));
                    line[i++] = c[0];
                    // end of string
                    if (c[0] == '\n') {
                        break;
                    }
                }
            }
            // a command was entered
            if (strlen(line) > 1) {
                // get command data and execute it if it satifies conditions
                if (tokenize(line)) {
                    eval();
                }
            }
        }
    }
    // end of shell
    exit(0);
}
//--------------------------------------------------------------------------------------
// Line reading and detection of symbols
//--------------------------------------------------------------------------------------
int tokenize(char *line) {
    char *p = line;
    int i = 0, quote;
    tokens = NULL;
    token_count = 0;
    // move to the first non-whitespace character
    while (isspace(*p)) {
        // check if they're all equal to spaces
        if (*(p++) == '\n') {
            return 0;
        }
    }
    // check if it's a comment
    if (*p == '#') {
        return 0;
    }
    // parse the line
    while (i == 0) {
        // check for quotes
        quote = 0;
        if (*p == '"') {
            quote = 1;
            p++;
        }
        // start of the symbol
        tokens = (char **) realloc(tokens, (token_count+1) * sizeof(char *));
        tokens[token_count++] = p;
        while (1) {
            // symbol in quotes
            if (quote == 1 && *p == '"') {
                *(p++) = '\0';
                // end of the line
                if (*(p++) == '\n' || *(p+1) == '\n') {
                    i = 1;
                }
                break;
            }
            // regular symbol
            else if (quote == 0 && isspace(*p)) {
                // end of the line
                if (*p == '\n' || *(p+1) == '\n') {
                    i = 1;
                }
                *p = '\0';
                p++;
                break;
            }
            // new character
            p++;
        }
    }
    return 1;
}
//--------------------------------------------------------------------------------------
// Command execution
//--------------------------------------------------------------------------------------
void eval() {
    int i = token_count-1;
    opt = (int *) calloc(3, sizeof(int));
    // process in background
    if (strcmp(tokens[i], "&") == 0) {
        opt[2] = 1;
        i--;
    }
    // save the descriptor state
    int j, fd[2];
    for (j = 0; j < 2; j++) {
        if ((fd[j] = dup(j)) < 0) {
            perror("dup");
        }
    }
    // redirection of output
    if (tokens[i][0] == '>') {
        opt[1] = 1;
        int fdout;
        if ((fdout = open(&tokens[i--][1], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
            perror("open");
        } else if (dup2(fdout, 1) < 0) {
            perror("dup2");
        }
    }
    // redirection of input
    if (tokens[i][0] == '<') {
        opt[0] = 1;
        int fdin;
        if ((fdin = open(&tokens[i--][1], O_RDONLY)) < 0) {
            perror("open");
        } else if (dup2(fdin, 0) < 0) {
            perror("dup2");
        }
    }
    //-------------------------------------------------------------------------------
    // INTERNAL COMMANDS
    //-------------------------------------------------------------------------------
    char *com = tokens[0];
    // NAME
    if (strcmp(com, "name") == 0) {
        fun_name(i);
    // HELP
    } else if (strcmp(com, "help") == 0) {
        fun_help();
    // STATUS
    } else if (strcmp(com, "status") == 0) {
        fun_status();
    // EXIT
    } else if (strcmp(com, "exit") == 0) {
        fun_exit(i);
    // PRINT
    } else if (strcmp(com, "print") == 0) {
        fun_print(i);
    // ECHO
    } else if (strcmp(com, "echo") == 0) {
        fun_echo(i);
    // PID
    } else if (strcmp(com, "pid") == 0) {
        if (opt[2] == 0) {
            fun_pid();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_pid();
                exit(0);
            }
        }
    // PPID
    } else if (strcmp(com, "ppid") == 0) {
        if (opt[2] == 0) {
            fun_ppid();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_ppid();
                exit(0);
            }
        }
    // DIR
    } else if (strcmp(com, "dir") == 0) {
        fun_dir(i);
    // DIRWHERE
    } else if (strcmp(com, "dirwhere") == 0) {
        if (opt[2] == 0) {
            fun_dirwhere();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_dirwhere();
                exit(0);
            }
        }
    // DIRMAKE
    } else if (strcmp(com, "dirmake") == 0) {
        if (opt[2] == 0) {
            fun_dirmake();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_dirmake();
                exit(0);
            }
        }
    // DIRREMOVE
    } else if (strcmp(com, "dirremove") == 0) {
        if (opt[2] == 0) {
            fun_dirremove();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_dirremove();
                exit(0);
            }
        }
    // DIRLIST
    } else if (strcmp(com, "dirlist") == 0) {
        if (opt[2] == 0) {
            fun_dirlist(i);
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_dirlist(i);
                exit(0);
            }
        }
    // LINKHARD
    } else if (strcmp(com, "linkhard") == 0) {
        if (opt[2] == 0) {
            fun_linkhard();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_linkhard();
                exit(0);
            }
        }
    // LINKSOFT
    } else if (strcmp(com, "linksoft") == 0) {
        if (opt[2] == 0) {
            fun_linksoft();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_linksoft();
                exit(0);
            }
        }
    // LINKREAD
    } else if (strcmp(com, "linkread") == 0) {
        if (opt[2] == 0) {
            fun_linkread();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_linkread();
                exit(0);
            }
        }
    // LINKLIST
    } else if (strcmp(com, "linklist") == 0) {
        if (opt[2] == 0) {
            fun_linklist();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_linklist();
                exit(0);
            }
        }
    // UNLINK
    } else if (strcmp(com, "unlink") == 0) {
        if (opt[2] == 0) {
            fun_unlink();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_unlink();
                exit(0);
            }
        }
    // RENAME
    } else if (strcmp(com, "rename") == 0) {
        if (opt[2] == 0) {
            fun_rename();
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_rename();
                exit(0);
            }
        }
    // CPCAT
    } else if (strcmp(com, "cpcat") == 0) {
        if (opt[2] == 0) {
            fun_cpcat(i);
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_cpcat(i);
                exit(0);
            }
        }
    // PIPES
    } else if (strcmp(com, "pipes") == 0) {
        if (opt[2] == 0) {
            fun_pipes(i);
        } else {
            int pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                fun_pipes(i);
                exit(0);
            }
        }
    //-------------------------------------------------------------------------------
    // EXTERNAL COMMANDS
    //-------------------------------------------------------------------------------
    } else {
        // foreground
        if (opt[2] == 0) {
            fun_exec_front(opt);
        // background
        } else {
            fun_exec_back(opt);
        }
    }
    // renew the descriptor state
    if (opt[1] == 1)
        fflush(stdout);
    for (j = 0; j < 2; j++) {
        if (dup2(fd[j], j) < 0) {
            perror("dup2");
        }
        if (close(fd[j]) < 0) {
            perror("close");
        }
    }
}
//-----------------------------------------------------------------------------------
// Set current directory
//-----------------------------------------------------------------------------------
void set_cwd() {
    long size = pathconf(".", _PC_PATH_MAX);
    cwd = (char *) malloc((size_t) size);
    if (getcwd(cwd, (size_t) size) == NULL) {
        int e = errno;
        perror("getcwd");
        exit(e);
    }
}
//-----------------------------------------------------------------------------------
// Print or change shell name
//-----------------------------------------------------------------------------------
void fun_name(int args) {
    if (args == 1) {
        name = tokens[1];
    } else {
        printf("%s\n", name);
    }
}
//-----------------------------------------------------------------------------------
// Print help
//-----------------------------------------------------------------------------------
void fun_help() {
    char commands[][10] = {
        "name", "help", "status", "exit", "print", "echo", "pid", "ppid", "dir",
        "dirwhere", "dirmake", "dirremove", "dirlist", "linkhard", "linksoft",
        "linkread", "linklist", "unlink", "rename", "cpcat", "pipes"
    };
    char description[][32] = {
        "Print or change shell name", "Print short help", "Print last command status",
        "Exit from shell", "Print arguments", "Print arguments and newline", "Print PID",
        "Print PPID", "Change directory", "Print current working directory",
        "Make directory", "Remove directory", "List directory", "Create hard link",
        "Creat symbolic/soft link", "Print symbolic link target", "Print hard links",
        "Unlink file", "Rename file", "Copy file", "Create pipeline"};
    int i;
    for (i = 0; i < 21; i++) {
        printf("%9s - %s\n", commands[i], description[i]);
        fflush(stdout);
    }
}
//-----------------------------------------------------------------------------------
// Print last output status of a foreground process
//-----------------------------------------------------------------------------------
void fun_status() {
    printf("%d\n", status);
}
//-----------------------------------------------------------------------------------
// Exit
//-----------------------------------------------------------------------------------
void fun_exit(int args) {
    int stat = 0;
    if (args == 1) {
        stat = atoi(tokens[1]);
    }
    exit(stat);
}
//-----------------------------------------------------------------------------------
// Print without new line
//-----------------------------------------------------------------------------------
void fun_print(int args) {
    int i;
    for (i = 1; i <= args; i++) {
        printf("%s", tokens[i]);
        if (i < args) {
            printf(" ");
        }
    }
    fflush(stdout);
}
//-----------------------------------------------------------------------------------
// Print with new line
//-----------------------------------------------------------------------------------
void fun_echo(int args) {
    int i;
    for (i = 1; i <= args; i++) {
        printf("%s", tokens[i]);
        if (i < args) {
            printf(" ");
        }
    }
    printf("\n");
}
//-----------------------------------------------------------------------------------
// Print PID
//-----------------------------------------------------------------------------------
void fun_pid() {
    printf("%d\n", getpid());
}
//-----------------------------------------------------------------------------------
// Print PPID
//-----------------------------------------------------------------------------------
void fun_ppid() {
    printf("%d\n", getppid());
}
//-----------------------------------------------------------------------------------
// Change directory
//-----------------------------------------------------------------------------------
void fun_dir(int args) {
    char *path = "/";
    if (args == 1) {
        path = tokens[1];
    }
    if (chdir(path) < 0) {
        perror("dir");
    }
}
//-----------------------------------------------------------------------------------
// Print current directory
//-----------------------------------------------------------------------------------
void fun_dirwhere() {
    set_cwd();
    printf("%s\n", cwd);
}
//-----------------------------------------------------------------------------------
// Create a new directory
//-----------------------------------------------------------------------------------
void fun_dirmake() {
    if (mkdir(tokens[1], S_IRWXU) < 0) {
        perror("dirmake");
    }
}
//-----------------------------------------------------------------------------------
// Delete a directory
//-----------------------------------------------------------------------------------
void fun_dirremove() {
    if (rmdir(tokens[1]) < 0) {
        perror("dirremove");
    }
}
//-----------------------------------------------------------------------------------
// Print files in a directory
//-----------------------------------------------------------------------------------
void fun_dirlist(int args) {
    // set current directory
    set_cwd();
    char *path = cwd;
    if (args == 1) {
        path = tokens[1];
    }
    // process the directory
    DIR *dirp;
    struct dirent *entry;
    // open the directory
    if ((dirp = opendir(path)) == NULL) {
        perror("opendir");
    }
    // read files in the directory
    int i = 0;
    while (1) {
        errno = 0;
        if ((entry = readdir(dirp)) != NULL) {
            // reading was successful
            if (i++ > 0) {
                printf("  ");
            }
            printf("%s", entry->d_name);
        } else {
            // reading was unsuccessful
            if (errno != 0) {
                perror("readdir");
                return;
            // no more files in the directory, so we close it
            } else {
                printf("\n");
                if (closedir(dirp) < 0) {
                    perror("closedir");
                }
                break;
            }
        }
    }
}
//-----------------------------------------------------------------------------------
// Create a hard link
//-----------------------------------------------------------------------------------
void fun_linkhard() {
    if (link(tokens[1], tokens[2]) < 0) {
        perror("linkhard");
    }
}
//-----------------------------------------------------------------------------------
// Create a soft link
//-----------------------------------------------------------------------------------
void fun_linksoft() {
    if (symlink(tokens[1], tokens[2]) < 0) {
        perror("linksoft");
    }
}
//-----------------------------------------------------------------------------------
// Print the target of the soft link
//-----------------------------------------------------------------------------------
void fun_linkread() {
    char path[512];
    int n;
    if ((n = readlink(tokens[1], path, sizeof(path))) < 0) {
        perror("linkread");
    } else {
        path[n] = '\0';
        printf("%s\n", path);
    }
}
//-----------------------------------------------------------------------------------
// Print all links to the file
//-----------------------------------------------------------------------------------
void fun_linklist() {
    // set current directory
    set_cwd();
    char *path = cwd;
    // find file's inode no.
    struct stat file;
    if (stat(tokens[1], &file) < 0) {
        perror("stat");
    }
    ino_t inode = file.st_ino;
    // find files with same inode no. as the given file's
    DIR *dirp;
    struct dirent *entry;
    // open the directory
    if ((dirp = opendir(path)) == NULL) {
        perror("opendir");
    }
    // read the files from the directory and print those with same inode no.
    int i = 0;
    while (1) {
        errno = 0;
        if ((entry = readdir(dirp)) != NULL) {
            // reading successful
            if (entry->d_ino == inode) {
                if (i++ > 0) printf("  ");
                printf("%s", entry->d_name);
            }
        } else {
            // reading unsuccessful
            if (errno != 0) {
                perror("readdir");
                return;
            // no more files in the directory, so we close it
            } else {
                printf("\n");
                if (closedir(dirp) < 0) {
                    perror("closedir");
                }
                break;
            }
        }
    }
}
//-----------------------------------------------------------------------------------
// Delete a file
//-----------------------------------------------------------------------------------
void fun_unlink() {
    if (unlink(tokens[1]) < 0) {
        perror("unlink");
    }
}
//-----------------------------------------------------------------------------------
// Rename a file
//-----------------------------------------------------------------------------------
void fun_rename() {
    if (rename(tokens[1], tokens[2]) < 0) {
        perror("rename");
    }
}
//-----------------------------------------------------------------------------------
// Copy a file or print its content to the standard output
//-----------------------------------------------------------------------------------
void fun_cpcat(int args) {
    int fdin = 0;
    // open the descriptor: input from file
    if (args >= 1 && strcmp(tokens[1], "-") != 0) {
        if ((fdin = open(tokens[1], O_RDONLY)) < 0) {
            perror("open");
            return;
        }
    }
    int fdout = 1;
    // open the descriptor: output to file
    if (args == 2 && strcmp(tokens[2], "-") != 0) {
        if ((fdout = open(tokens[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
            perror("open");
            return;
        }
    }
    // copy the content of the input file into the output file
    char *buffer = (char *) malloc(1024 * sizeof(char));
    char c[1];
    int i = 0, j = 1;
    while (read(fdin, c, 1) > 0) {
        buffer[i++] = c[0];
        if (i % 1024 == 0) {
            buffer = (char *) realloc(buffer, (++j) * 1024 * sizeof(char));
        }
    }
    if (write(fdout, buffer, i) < 0) {
        int e = errno;
        perror("write");
        exit(e);
    }
    // close the descriptors
    if (fdin != 0 && close(fdin) < 0) {
        int e = errno;
        perror("close");
        exit(e);
    }
    if (fdout != 1 && close(fdout) < 0) {
        int e = errno;
        perror("close");
        exit(e);
    }
}
//-----------------------------------------------------------------------------------
// Create pipeline
//-----------------------------------------------------------------------------------
void fun_pipes(int args) {
    // parse the pipeline commands
    int i, j = 0, k = 0;
    count = (int *) malloc(args * sizeof(int));
    split = (char ***) malloc(args * sizeof(char **));
    for (i = 1; i <= args; i++) {
        split[j] = NULL;
        char *p = strtok(tokens[i], " ");
        while (p != NULL) {
            split[j] = (char **) realloc(split[j], (j+1) * sizeof(char *));
            split[j][k] = (char *) malloc((strlen(p)+1) * sizeof(char));
            strcpy(split[j][k++], p);
            p = strtok(NULL, " ");
        }
        split[j] = (char **) realloc(split[j], (j+1) * sizeof(char *));
        split[j][k] = NULL;
        count[j] = k-1;
        j++;
        k = 0;
    }
    // create a pipeline
    int fd1[2];
    int fd2[2];
    if (pipe(fd1) < 0) {
        perror("pipe");
    }
    pipe_start(fd1, 0);
    for (i = 1; i < args-1; i++) {
        if (pipe(fd2) < 0) {
            perror("pipe");
        }
        pipe_middle(fd1, fd2, i);
        memcpy(fd1, fd2, 2*sizeof(int));
    }
    if (args < 3) {
        pipe_end(fd1, i);
    } else {
        pipe_end(fd2, i);
    }
    fflush(stdout);
}
//-----------------------
// Start of pipeline
//-----------------------
void pipe_start(int fd[], int i) {
    int pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // redirect child's standard output to the writing end of the 1st pipe
        if (dup2(fd[1], 1) < 0) {
            perror("dup2");
        }
        // close the descriptors
        if (close(fd[0]) < 0) {
            perror("close");
        }
        if (close(fd[1]) < 0) {
            perror("close");
        }
        if (fun_exec_internal(i) == 1) {
            exit(EXIT_SUCCESS);
        }
        execvp(split[i][0], split[i]);
        perror("execvp");
        exit(EXIT_FAILURE);
     }
}
//-----------------------
// Middle of pipeline
//-----------------------
void pipe_middle(int fd1[], int fd2[], int i) {
    int pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // redirect child's standard input to the reading end of the 1st pipe
        if (dup2(fd1[0], 0) < 0) {
            perror("dup2");
        }
        // redirect child's standard output to the writing end of the 2nd pipe
        if (dup2(fd2[1], 1) < 0) {
            perror("dup2");
        }
        // close the descriptors
        if (close(fd1[0]) < 0) {
            perror("close");
        }
        if (close(fd1[1]) < 0) {
            perror("close");
        }
        if (close(fd2[0]) < 0) {
            perror("close");
        }
        if (close(fd2[1]) < 0) {
            perror("close");
        }
        if (fun_exec_internal(i) == 1) {
            exit(EXIT_SUCCESS);
        }
        execvp(split[i][0], split[i]);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // close the descriptors
        if (close(fd1[0]) < 0) {
            perror("close");
        }
        if (close(fd1[1]) < 0) {
            perror("close");
        }
    }
}
//-----------------------
// End of pipeline
//-----------------------
void pipe_end(int fd[], int i) {
    int pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // redirect child's standard input to the reading end of the 2nd pipe
        if (dup2(fd[0], 0) < 0) {
            perror("dup2");
        }
        // close the descriptors
        if (close(fd[0]) < 0) {
            perror("close");
        }
        if (close(fd[1]) < 0) {
            perror("close");
        }
        if (fun_exec_internal(i) == 1) {
            exit(EXIT_SUCCESS);
        }
        execvp(split[i][0], split[i]);
        perror("execvp");
        exit(EXIT_FAILURE);
     } else {
         // close the descriptors
         if (close(fd[0]) < 0) {
             perror("close");
         }
         if (close(fd[1]) < 0) {
             perror("close");
         }
         while (waitpid(-1, NULL, 0) > 0) { }
    }
}
//-----------------------------------------------------------------------------------
// Execute external command in foreground
//-----------------------------------------------------------------------------------
void fun_exec_front(int *opt) {
    int n;
    int pid = fork();
    // error
    if (pid < 0) {
        perror("fork");
    // child
    } else if (pid == 0) {
        n = token_count - (opt[0]+opt[1]);
        tokens = (char **) realloc(tokens, (n+1) * sizeof(char *));
        tokens[n] = NULL;
        execvp(tokens[0], tokens);
        perror("execvp");
        exit(EXIT_FAILURE);
    // parent
    } else {
        // wait until the child exits
        int stat;
        if (waitpid(pid, &stat, 0) < 0) {
            perror("waitpid");
        }
        // check if child exited with exit() and obtain the exit status
        if (WIFEXITED(stat) > 0) {
            status = WEXITSTATUS(stat);
        }
    }
}
//-----------------------------------------------------------------------------------
// Execute external command in background
//-----------------------------------------------------------------------------------
void fun_exec_back(int *opt) {
    int n;
    int pid = fork();
    // error
    if (pid < 0) {
        perror("fork");
    // child
    } else if (pid == 0) {
        n = token_count-(opt[0]+opt[1])-1;
        tokens = (char **) realloc(tokens, (n+1) * sizeof(char *));
        tokens[n] = NULL;
        execvp(tokens[0], tokens);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}
//-----------------------------------------------------------------------------------
// Catch daemon exit statuses
//-----------------------------------------------------------------------------------
void handler(int signum) {
    waitpid(WAIT_ANY, NULL, WNOHANG);
}
//-----------------------------------------------------------------------------------
// Call internal function in pipeline
//-----------------------------------------------------------------------------------
int fun_exec_internal(int index) {
    tokens = split[index];
    char *com = tokens[0];
    int i = count[index];
    // NAME
    if (strcmp(com, "name") == 0) {
        fun_name(i);
    	return 1;
    // HELP
    } else if (strcmp(com, "help") == 0) {
        fun_help();
	    return 1;
    // STATUS
    } else if (strcmp(com, "status") == 0) {
        fun_status();
    	return 1;
    // EXIT
    } else if (strcmp(com, "exit") == 0) {
        fun_exit(i);
	    return 1;
    // PRINT
    } else if (strcmp(com, "print") == 0) {
        fun_print(i);
    	return 1;
    // ECHO
    } else if (strcmp(com, "echo") == 0) {
        fun_echo(i);
	    return 1;
    // PID
    } else if (strcmp(com, "pid") == 0) {
    	fun_pid();
    	return 1;
    // PPID
    } else if (strcmp(com, "ppid") == 0) {
    	fun_ppid();
    	return 1;
    // DIR
    } else if (strcmp(com, "dir") == 0) {
        fun_dir(i);
	    return 1;
    // DIRWHERE
    } else if (strcmp(com, "dirwhere") == 0) {
	    fun_dirwhere();
	    return 1;
    // DIRMAKE
    } else if (strcmp(com, "dirmake") == 0) {
    	fun_dirmake();
    	return 1;
    // DIRREMOVE
    } else if (strcmp(com, "dirremove") == 0) {
    	fun_dirremove();
    	return 1;
    // DIRLIST
    } else if (strcmp(com, "dirlist") == 0) {
    	fun_dirlist(i);
    	return 1;
    // LINKHARD
    } else if (strcmp(com, "linkhard") == 0) {
    	fun_linkhard();
    	return 1;
    // LINKSOFT
    } else if (strcmp(com, "linksoft") == 0) {
    	fun_linksoft();
    	return 1;
    // LINKREAD
    } else if (strcmp(com, "linkread") == 0) {
    	fun_linkread();
	    return 1;
    // LINKLIST
    } else if (strcmp(com, "linklist") == 0) {
    	fun_linklist();
	    return 1;
    // UNLINK
    } else if (strcmp(com, "unlink") == 0) {
    	fun_unlink();
    	return 1;
    // RENAME
    } else if (strcmp(com, "rename") == 0) {
    	fun_rename();
    	return 1;
    // CPCAT
    } else if (strcmp(com, "cpcat") == 0) {
    	fun_cpcat(i);
    	return 1;
    }
    return 0;
}
