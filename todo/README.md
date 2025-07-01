# Project 1

## header (shell.hpp)

### pipeStruct
```=cpp
struct pipeStruct {
    int OutCommandIndex;       /* same as key in pipeMap */
    int fd[2];
    vector<pid_t> relate_pids;
    int startCommandType;
}
```
:::info
Create a pipe for each command’s input
:::
Record the status of each pipe
1. `OutCommandIndex`: Records which command this pipe outputs to
2. `fd[2]`: Stores the pipe
3. `relate_pids`: Which processes are related to this pipe’s input
4. `startCommandType`: Adjusts the order between number pipes and ordinary pipes

### Info
```=cpp
struct Info{
    bool bg;
    vector<int> op;
    vector<int> opOrder;
    vector<vector<string>> argv;
}
```
Stores the input commands and their execution order
1. `bg`: Whether to run in background
2. `op`: Pipe type of each command
    ```=hpp
    END_OF_COMMAND : 0
    PIPE           : 1
    NUM_PIPE       : 2
    NUM_PIPE_ERR   : 3
    OUT_RD         : 4
    ```
3. `opOrder`: Which command each command should pipe to
    ```=hpp
    NOT_PIPE        : 0
    NOT_NUMBER_PIPE : 1
    other(some n>0) : pipe to which command (n-th)
    ```
4. `argv`: Command content, each command is stored as a `vector<string>`

For example, Input command
`% commandA -flagA | commandB |3 commandC -flagC > file`
```=cpp
bg = false // no '&'
argv = {                       op = {                oporder = {
    {'commandA', '-flagA'},        PIPE,                 0 + NOT_PIPE,
    {'commandB'},                  NUM_PIPE_ERR,         1 + 3,
    {'commandC', '-flagC'},        OUT_RD,               NOT_PIPE
    {'file'}                       END_OF_COMMAND
}                              }                     }
```

## source code (shell.cpp)

### sigchld_handler
```=cpp
signal(SIGCHLD, sigchld_handler);
void sigchld_handler(int signo) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}
```
Registers a function to handle SIGCHLD signal when a process terminates.
ses `WNOHANG` flag to avoid blocking—returns immediately if no child is to be waited on.

### typePrompt
```=cpp
input argument : showPath(bool)
return type    : void
```
Displays the shell prompt symbol.
Argument decides whether to show host name and current working directory.

### readCommand
```=cpp
input argument : &info (struct Info) /* by reference*/
                 totalCommandCount (const int)
return type    : int
```
Parses input arguments and stores them into `info`.
Returns how many commands were read (output redirection also counts as one).

Handles number pipe and ordinary pipe counter issues — single line
```=bash
% removetag test.html |2 removetag  test.html | number
% number
```
`removetag` in beginning must pipe to `number` on the next line.
Approach: On reading `'|'` (`op = PIPE`), check if there is a numbered pipe (`op = NUM_PIPE`) on the same line that exceeds the current ordinary pipe count.

### builtInComand
```=cpp
input argument : info (struct Info)
return type    : int
```
Checks whether the command is a built-in (`printenv`, `setenv`, `exit`, `cd`), and executes if so.
Returns whether it was a built-in; `exit` returns `-1`.

### executeCommand
```=cpp
input argument : info (pipeStruct)
                 pipeMap (man<int,pipStruct>)
                 currentCommandStart (const int)
                 totalCommandCount   (const int)
return type    : void
```
Executes the command using `info`.
 
For each command:
1. Maintain `pipeMap`:
    If the target pipe (output to current command) doesn't exist yet, create it.

2. `fork()`
   2-1. child process: deal the pipe and executes the command
   > 1. dup2 pipe-out (output to current command) if it exists (else use `STDIN`)
   > 2. Determine if output goes to pipe, file, or `STDOUT`
   > 3. Use `execvp` to execute
       
   2-2. parent process: Closes pipe and waits
   > 1. Collects related process PIDs
   > 2. Closes related pipes
   > 3. If output is to file or STDOUT, wait for process
   > 4. Else, it can run ahead (large outputs may block)

3. `while (waitpid(-1, &status, WNOHANG)>0);`
    Terminates all zombie processes

### main function
1. Set environment variable `PATH="bin:."`
2. print prompt `%`
3. call `readCommand` and get #commands
4. update counter
5. Handle number vs ordinary pipe counter issues — multi-line
    ```=bash
    % removetag test.html |2
    % removetag test.html | number
    % number
    ```
    `removetag` in beginning in beginning `number`.
    Approach: Before executing the next command, check whether the line includes an ordinary pipe (`'|'`).
    If so, check whether the input pipe type is a number pipe and if it exceeds the current execution line.
    If both true, redirect output of that pipe to next command (use new `pipeMap`).
6. If built-in command is `exit`, go to step 9
7. call `executecommand`
8. Return to step 2
9. Exit

### demo part
Choose one of eight questions

Question 6: Ignore N line command (`%N`)
```=bash
% ls |5
% ls |2
% ls %2            # ignore next 2 lines, output result now  
bin
test.html
% cat              # ignore
% number test.html # ignore
% cat              # from ls|2, not from ls |5
bin
test.html
% noop
% number           # from ls |5
    1 bin
    2 test.html
% exit
```
~~Originally my demo was wrong—I piped `ls |5` to `cat`, but result was same.~~
1. Add `IGNORE:5` to `op` options
2. On reading `%`, `info.op=IGNORE`, `info.opOrder=` number after `%`
3. `pipeMap` must be pushed with `IGNORE` (handled with multi-line pipe fix) ~~and I forgot it in demo~~
   :::warning
   At `ignore_idx++`, **when condition is met, i.e., during ignore period**, move ignore count forward as well. since `command A | command B` counts as 1 command)
   Single line doesn’t need this fix because `command %N` outputs directly and doesn’t connect to pipe.
   :::
4. If current c`ommand index <= ignore_idx`, skip execution
5. During execution, if `op` is `IGNORE`, assign `opOrder` to `ignore_idx` (a global variable)

Question 7: Implement "+", no space between "+" (captured by accident)
```=bash
% ls |1+2
% noop
% noop
% number
    1 bin
    2 test.html
% # rest not captured
```
... just ... handle token parsing in `info.opOrder` to support `+`
```=cpp
size_t prev_start = 1;
for (size_t i = 1; i < token.size(); ++i) {
    if (token[i] == '+') {
        opOrder += stoi(token.substr(prev_start, i - prev_start));
        prev_start = i + 1;
    }
}
if (token.size() != 1) {
    opOrder += stoi(token.substr(prev_start, token.size() - prev_start));
}
```

## Body (remain.cpp)
:::danger
Create a pipe for each command’s output
:::
When multiple commands input to the same command, it will overwrite

:::warning
Use merge pipe to combine
:::
Works with small outputs, but large outputs like `manyblessings` will overflow the buffer
```=cpp
int mergePipe[2], buf[4096];
pipe(mergePipe);
while(read(inpipe, buf, sizeof(buf)) > 0) {
    write(outpipe, buf, sizeof(buf));
}
```
