# LshShell

A Unix-like shell written completely from scratch in C.

LshShell is a personal systems programming project built to understand how real Unix shells work internally. Rather than wrapping existing shell implementations, it implements the core components itself: lexical analysis, parsing into an Abstract Syntax Tree (AST), command execution, pipelines, redirections, process groups, job control, and signal handling.

---

## Features

### Command Execution

* Execute external programs using `fork()` and `execve()`
* Built-in command support
* PATH resolution
* Shell variable assignments
* Environment variable expansion

### Parsing

* Custom lexer
* Recursive descent parser
* Abstract Syntax Tree (AST)
* Operator precedence
* Recursive execution engine
* Logical AND (`&&`)

### Pipes, Redirection & Operators

* Arbitrarily long pipelines

```sh
echo hello | grep h | wc
```

* Input redirection

```sh
sort < input.txt
```

* Output redirection

```sh
echo hello > output.txt
```

* Output append

```sh
echo world >> output.txt
```

### Job Control

* Background execution

```sh
sleep 30 &
```

* Foreground jobs
* Background jobs
* Process groups
* Terminal ownership management
* `jobs`
* `fg`
* `bg`
* Ctrl+C (`SIGINT`)
* Ctrl+Z (`SIGTSTP`)
* Ctrl+\ (`SIGQUIT`)

### Shell Features

* Command history
* Prompt displaying current working directory
* Exit status tracking (`$?`)
* Shell variables
* Environment variables

---

# Example Usage

Execute commands

```sh
[lsh]@~# ls
Desktop  Documents  Downloads
```

Pipelines

```sh
[lsh]@~# cat file.txt | grep main | wc
```

Redirection

```sh
[lsh]@~# echo Hello > hello.txt
[lsh]@~# cat hello.txt
Hello
```

Logical AND

```sh
make && ./program
```

Background jobs

```sh
[lsh]@~# sleep 20 &
[1] 4213

[lsh]@~# jobs
[1] Running (sleep 20)

[lsh]@~# fg id

[lsh]@~# bg id
Job: id |id| has been started again
```

* If id not specified in fg or bg it defaults to 1

Shell variables

```sh
[lsh]@~# name=Kartik
[lsh]@~# echo $name
Kartik

[lsh]@~# export name
[lsh]@~# unset name
```

Environment vairbales

```sh
[lsh]@~# echo $SHELL
/usr/local/bin/lsh

[lsh]@~# export HOME=/home/person
```

History

```sh
[lsh]@~# history
```

---

### Complex Example

Commands can freely combine logical operators, pipelines, redirections, and background execution.

```sh
[lsh]@~# cat input.txt | grep main | sort > output.txt && echo Done

````

---

# Architecture

```
               User Input
                    │
                    ▼
                  Read
                    │
                    ▼
                  Lexer
                    │
                    ▼
            Recursive Parser
                    │
                    ▼
          Abstract Syntax Tree
                    │
                    ▼
                Executor
          ┌─────────┴─────────┐
          ▼                   ▼
      Built-ins         External Commands
          │                   │
          └─────────┬─────────┘
                    ▼
             Job Control Layer
                    │
                    ▼
              Linux Processes
```

---

# Parsing

Commands are first tokenized and then parsed into an Abstract Syntax Tree.

For example,

```sh
echo hello | grep h && wc
```

produces an AST conceptually similar to

```
          &&
        /    \
     PIPE     wc
    /    \
 echo    grep
```

This structure naturally preserves operator precedence and allows recursive execution.

---

# Pipeline Execution

Pipelines are flattened into an ordered list of commands.

For each pipeline the shell:

1. Creates all required pipes.
2. Forks one child process for every command.
3. Places every child into the same process group.
4. Connects stdin/stdout using `dup2()`.
5. Executes built-ins or external commands.
6. Waits for the foreground process group to finish.

This enables pipelines of arbitrary length such as

```sh
cat file.txt | grep main | sort | uniq | wc
```

---

# Job Control

Foreground jobs receive terminal ownership using `tcsetpgrp()`.

Background jobs are tracked internally using process groups.

Signals such as:

* `SIGINT`
* `SIGTSTP`
* `SIGCONT`
* `SIGQUIT`

are forwarded to entire process groups, allowing every process in a pipeline to stop or continue together.

---

# Project Structure

```
.
├── include/
├── src/
│   ├── exec/
│   ├── parse/
│   ├── shell/
│   └── main.c
├── Makefile
└── README.md
```

---

# Building

Clone the repository

```bash
git clone https://github.com/<your-username>/LshShell.git
cd LshShell
```

Build

```bash
make run
```

Run

```bash
lsh
```

Enjoy!

---

# Technologies

* C
* POSIX API
* Linux Process Management
* Signals
* Pipes
* File Descriptors
* Process Groups
* Terminal Control

---

# Future Improvements

* Command substitution (`$(...)`)
* Wildcard expansion (`*`)
* Quoting improvements
* Here-documents (`<<`)
* Aliases
* Shell scripting support
* Auto-completion
* Persistent history
* Improved POSIX compatibility
* Making it faster and efficient

---

# Fun Detail

* Added a shell variable called "MADEBY"
```sh
[lsh]@~# echo $MADEBY
Light-shell(lsh) made by Kartik Sandhu!
```
---

## License

This project is licensed under the MIT License.
