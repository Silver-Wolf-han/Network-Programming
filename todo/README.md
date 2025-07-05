# Project 3

## header (np_multi_proc.hpp)
Merge `npshell.hpp` and `np_single_proc.hpp` from Project 2 (~~Global Variable haha~~)

### struct UserInfo
```=cpp
// Add pid (used for sending SIG)
pid_t pid;
// Remove totalCommandCount, relate_pids, startCommandType, who_block_me
// Because it's concurrent connect-oriented, no need to keep track
// who_block_me is now handled using shared memory
```

## source code (np_multi_proc.cpp)

### Global Variable
1. `int UserInfo_fd, MSG_fd, UserPipeMatrix_fd`
    Used for mapping shared memory
    
2. `struct UserInfo *UserInfo`
    Stores user information (with shared memory)

3. `char *msg`
    Stores broadcast messages (with shared memory)

4. `int *UserPipeMatrix`
    Records the state of pipes between users (mainly to determine if FIFO can be performed)
    A 2D array(Conceptually `UserPipeMartirx[i][j]`, actually represented as `UserPipeMatrix[i * MAC_CLIENT + j]` indicating the status of user i to user j)
    -1: Not activated yet (either User i or User j is not present)
     0: oth users are present, FIFO can be created but hasn't been
    \>0: User i has opened the FIFO, return value from `open()` is stored here, User j reads from it.
    -2: User i block User j，transmission not allowed
    -3: Temporary state, User i just sent to User j, User j must `open()` and read immediately or User i will be blocked
    **Possible overflow risk: Sending and opening are stored in the same place. What happens if a very large file is sent?**
    > ❗ **Danger:**
    > 
    > From project2 demo
    > The issue not mention above, is just be blocked. For example, `manyblessings A >2` is an END_COMMAND, but it's meant to be sent to others and must not be blocked, so add a condition: if there's to_user_pipe, do not wait.
    >
    > Still has other issues... like resource errors... just ignore it, let it die.

5. `size_t user_idx_glob`
    Keeps track of the process owner. Why? Because you only know the user when the signal is received.
    (~~Technically, all `size_t user_idx` could be removed, but I'm too lazy~~)

### concurentConnectionOrientServer
```=cpp
input argument : port(int)
return type    : void
```
Mostly the same as Project 2 - Part 1.
Differences:
1. Registers three extra functions at the start:
    ```=cpp
    signal(SIGINT, release_share_memory)
    // Clean up shared memory
    signal(SIGUSR1, sendMsg)
    // broadcast msg
    signal(SIGUSR2, receiveFifo)
    // Receive FIFO
    ```

2. Before entering the accept loop, initialize shared memory:
    ```=cpp
    int memory_fd = shm_open("memory", O_CREAT | O_RDWR, 0666);
    ftruncate(memory_fd, SIZE);
    type *var = (type *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memory_fd, 0);
    // var init, for example
    memset(var, 0, SIE)
    ```

3. Before `fork()` after `accept`, identify `user_idx, user_idx_glob`
    ~~putting it after fork() might be better~~

4. `fork()` - parent:
    Prepare this client’s related data

5. `fork()` - child
    After parent finishes, enter `npshellInit(), npshellLoop(user_idx)`
    When `npshellLoop()`returns, it means user logged out—clean up `UserInfo`, shared memory, FIFO


### sendMsg
```=cpp
input argument : signo(int)
return type    : void
```
On receiving `SIGUSR1`, send the message

### receiveFifo
```=cpp
input argument : signo(int)
return type    : void
```
On receiving `SIGUSR2`, fetch FIFO (by value `-3`)
Must handle immediately, or sender will be blocked (`open` here should use `O_NONBLOCK`)

### release_share_memory
```=cpp
input argument : signo(int)
return type    : void
```
On receiving `SIGINT`, release memory


A large portion of the functions are very similar to Project 2 Part 2
The main difference is whether the data comes from passed parameters or from shared memory

A Large Difference
### executeCommand
User pipe logic is similar (~~watch out for index copying bugs: to_user_pipe, from_user_pipe... very error-prone~~)
0. How to distinguish if user exists? If `from_token_idx != 0 && from_user_pipe == -1`, then the source user doesn't exist

1. If command includes `>n`, need to create FIFO
    ```=cpp
    mkfifo(fifoName, 0666);
    UserPipeMatrix[user_idx * MAX_CLIENT + to_user_pipe] = -3
    kill(UserInfo[to_user_pipe].pid, SIGUSER2);
    int fd = open(fifo, O_WRONLY); // Without O_NONBLOCK, If with it will output to STDOUT directly
    dup2(fd, STDOUT_FILEENO);
    close(fd);
    ```

2. If command includes `<n`, i.e., receive from another user—read, directly from UserPipeMatrix[from_user_pipe * MAX_CLIENT + user_idx] because `SIGUSR2` already handled it.

3. After `fork()`, if FIFO is used, unlink it

### broadcast
```=cpp
input argument : broadcastMsg(string)
return value   : void
```
1. Clear shared memory msg
2. Fill it with `broadcastMsg`
3. Send `SIGUSR1` to all active users
4. **Sleep 1 second**, Why? When broadcasting consecutively, the latter will overwrite the former
    (~~A better solution might be using separate memory blocks for each message, but what about ordering?~~)

## Demo
Got the same question as part 2, lol

pick another question: firewall (takes ~10 mins, version that only blocks one IP)
```
% firewall 127.0.0.1

                        $ telnet [server IP] [Server Port] # From 127.0.0.1   
                        *** Error: You can not login. ***
                        $
% who
輸出
# Will not show the info of the user who tried to log in
```

write a full custom spec:
1. If some one has typed `firewall IP1`，than user log-in from `IP1` will not be allowed while output Error Msg
2. Besides, Active User will not know the user (also can not show in `who` or `tell`command)
3. allow `firewall` multi IP
4. Use `unfirewall IP1` to remove the restriction

**Allow firewall multi IP** => The only idea I have is to open multiple shared memory variables and fds (~~so bad~~)
Add two built-in functions to manage these shared memory variables
Open both the fds and vars as arrays, and check if the IP is on the blocklist before entering `np_init()`
