# REPL
Bash-like REPL implemented in C. Supports interactive and non-interactive mode.

## Installation & Usage
```
gcc repl.c -o repl
./repl

## Examples
```bash
mysh> name
mysh
mysh> name abrakadabra
abrakadabra>
abrakadabra>
abrakadabra> name mysh
mysh> print Simple shell
Preprosta lupinamysh> echo No quotes
No quotes
mysh> echo "With      quotes"
With      quotes
mysh> pid
23327
mysh> ppid
21953
mysh> # comment
mysh>               # comment
mysh> help
      name - Print or change shell name
      help - Print short help
     debug - Toggle debug mode
    status - Print last command status
      exit - Exit from shell
     print - Print arguments
      echo - Print arguments and newline
       pid - Print PID
      ppid - Print PPID
       dir - Change directory
  dirwhere - Print current working directory
   dirmake - Make directory
 dirremove - Remove directory
   dirlist - List directory
dirinspect - Inspect directory
  linkhard - Create hard link
  linksoft - Create symbolic/soft link
  linkread - Print symbolic link target
  linklist - Print hard links
    unlink - Unlink file
    rename - Rename file
    remove - Remove file or directory
     cpcat - Copy file
     pipes - Create pipeline
mysh> # files
mysh> dirwhere
/home/user/repl
mysh> dirmake test
mysh> dirmake test
dirmake: File exists
mysh> linkhard .bashrc test/bash-settings
mysh> dir test
mysh> dirwhere
/home/user/repl/test
mysh> linkhard bash-settings bash-conf
mysh> linksoft bash-settings bash-conf-soft
mysh> linksoft some/directory shortcut
mysh> dirlist
bash-settings  bash-conf-soft  bash-conf  shortcut  ..  .
mysh> linklist bash-conf
bash-settings  bash-conf
mysh> dirmake delete-me
mysh> unlink delete-me
unlink: Is a directory
mysh> dirremove delete-me
mysh> unlink bash-conf
mysh> unlink bash-conf
unlink: No such file or directory
mysh> rename bash-settings bash-conf
mysh> dirlist
bash-conf-soft  bash-conf  bliznjica  ..  .
mysh> # redirection
mysh> echo something >a.txt
mysh> cat a.txt
something
mysh> cpcat a.txt
something
mysh> cpcat a.txt b.txt
mysh> cat b.txt
something
mysh> cpcat - c.txt
Spring is here                  # CTRL+D to end
mysh> cat b.txt c.txt
something
Spring is here
mysh> cpcat <a.txt >d.txt
mysh> cat d.txt
something
mysh> # background processing
mysh> pid
27206
mysh> pid &
mysh> 27782

mysh> pid &
mysh> 27788

mysh> pid &
mysh> 27791

mysh> pid >pid.txt &
mysh> cpcat <pid.txt
27818
mysh> pid >pid.txt &
mysh> cpcat <pid.txt
28055
mysh> # pipelines
mysh> pipes "cat /etc/passwd" "wc -l"
249
mysh> pipes "cat /etc/passwd" "head -13" "tail -3" "wc -l"
3
mysh> exit 42
```
