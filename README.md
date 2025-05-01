# Project 3

## header (np_multi_proc.hpp)
把Project 2的`npshell.hpp`和`np_single_proc.hpp`融合(~~Global Variable哈哈~~)

### struct UserInfo
```=cpp
// 多加 pid (傳送SIG要用)
pid_t pid;
// 刪掉 totalCommandCount, relate_pids, startCommandType, who_block_me
// 因為是用Concurrent Connect-Oriented 所以不用紀錄
// who_block_me改用share memory
```

## source code (np_multi_proc.cpp)

### Global Variable
1. `int UserInfo_fd, MSG_fd, UserPipeMatrix_fd`
    用來Map Share memory的
    
2. `struct UserInfo *UserInfo`
    紀錄每個使用者的資訊 (有share memory)

3. `char *msg`
    紀錄廣播訊息 (有share memory)

4. `int *UserPipeMatrix`
    紀錄User之間Pipe的狀態，(主要是可不可以進行Fifo)
    二維陣列(概念上`UserPipeMartirx[i][j]` 實際上`UserPipeMatrix[i * MAC_CLIENT + j]`來表示User i 對 User j的狀態)
    -1: 還沒啟用(要馬 User i 不在要馬 User j 不在)
     0: 兩個User都在，可以塞Fifo但還沒塞
    \>0: User i 用`open`開好fifo，`open`的return值存在這裡， User j要拿的時候就從這裡拿
    -2: User i block User j，不給傳送
    -3: 暫時狀態，User i 剛剛傳給User j，User j要馬上去用`open`把它收進來，不然User i會block
    **好像有爆開的風險欸幹，送東西跟開東西放在同一個地方，如果送很大的檔案會不會出事?**
    :::danger
    From project2 demo
    會出事是出在被block住，因為`manyblessings A >2`這種東西是END_COMMAND，可是要丟給其他人，不可以被block住，所以要多判斷如果有to_user_pipe就不用等
    
    可是還是有其他問題欸.. resource 不夠 error.. 這個不管它了 管他去死
    :::

5. `size_t user_idx_glob`
    用來記錄這個process是誰，為甚麼呢，因為收到signal時才知道是誰
    (~~那理論上所有`size_t user_idx`都可以殺掉，但我懶得殺~~)

### concurentConnectionOrientServer
```=cpp
input argument : port(int)
return type    : void
```
大致跟Project 2 - Part 1一樣
不一樣的地方
1. 前面多註冊三個function
    ```=cpp
    signal(SIGINT, release_share_memory)
    // 那些共用記憶體殺掉
    signal(SIGUSR1, sendMsg)
    // broadcast msg
    signal(SIGUSR2, receiveFifo)
    // 收Fifo
    ```

2. 進入accept loop之前，初始化那些share memory
    ```=cpp
    int memory_fd = shm_open("memory", O_CREAT | O_RDWR, 0666);
    ftruncate(memory_fd, SIZE);
    type *var = (type *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memory_fd, 0);
    // var init, for example
    memset(var, 0, SIE)
    ```

3. accept之後 fork()之前，辨認一下`user_idx, user_idx_glob`
   ~~其實好像放在fork之後比較好~~
   
4. fork() - parent:
    幫忙把這個cleint的東西放好

5. fork() - child
    等parent把東西放好之後再進入`npshellInit(), npshellLoop(user_idx)`
    從`npshellLoop()`離開後表示使用者登，殺掉UserInfo，ShareMemory，FIFO


### sendMsg
```=cpp
input argument : signo(int)
return type    : void
```
收到`SIGUSR1`就送出訊息

### receiveFifo
```=cpp
input argument : signo(int)
return type    : void
```
收到`SIGUSR2`就去把Fifo收進來(by value `-3`)
要立刻處理，不然sender會block住(這裡的`open`要加`O_NONBLOCK`)

### release_share_memory
```=cpp
input argument : signo(int)
return type    : void
```
收到`SIGINT`就把記憶體放一放


後面有一大部分的function都和project 2 part 2長很像
部分差別就是從傳進來的parament拿東西還是從share memory拿東西而已

比較大差別的
### executeCommand
判斷user pipe之類的都很像(~~注意複製時的index，to_user_pipe，from_user_pipe，一直不小心貼錯就一大堆bug~~)
0. 怎麼區分User存不存在?，`from_token_idx!=0 && from_user_pipe==-1`表示要從不存在的東西拿

1. 如果command有`>n`丟出去的，要創造fifo
    ```=cpp
    mkfifo(fifoName, 0666);
    UserPipeMatrix[user_idx * MAX_CLIENT + to_user_pipe] = -3
    kill(UserInfo[to_user_pipe].pid, SIGUSER2);
    int fd = open(fifo, O_WRONLY); //沒有O_NONBLOCK，有的話直接輸出到STDOUT
    dup2(fd, STDOUT_FILEENO);
    close(fd);
    ```
2. 如果command有`<n`收別人的，直接去`UserPipeMatrix[from_user_pipe * MAX_CLIENT + user_idx]`收東西，因為收到`SIGUSR2`就處理完了

3. fork()完之後，如果有用到fifo要去把它unlink

### broadcast
```=cpp
input argument : broadcastMsg(string)
return value   : void
```
1. 把共用記憶體msg清空
2. 把broadcastMsg填進去
3. kill每個活著的人`SIGUSR1`
4. **等一秒**為什麼?連續broadcast的時候，後面的會把前面的給蓋掉
   (~~感覺比繫好的做法是分不同的msg用不同的記憶體，可是這樣先後順序呢?~~)

## Demo
抽到跟part 2一樣的 笑死

偷看到另一題firewall (要花十多分鐘，只允許封鎖一個的版本)
```
% firewall 127.0.0.1

                        $ telnet [server IP] [Server Port] #從127.0.0.1   
                        *** Error: You can not login. ***
                        $
% who
輸出
# 不會看到剛剛嘗試登入的資訊   
```

還是把完整的自訂spec寫下來吧
1. 已經在聊天室的某個人如果下`firewall 某IP`，則從`某IP`登入的人不能登入，輸出錯誤訊息
2. 除了不能登入，聊天室內的其他人也不會知道這個使用者的存在(`who`，`tell`等command都不會送給他)
3. 允許`firewall`多個IP
4. 用`unfirewall 某IP`來將某個IP移除firewall，如果不再則輸出錯誤訊息

**允許firewall多個IP** => 我只有想到開多個shm變數和fd(~~超爛~~)
多兩個built-in function來處理這兩個共用記憶體的變數
fd和變數都開成陣列，然後就登入`np_init()`前比較一下IP有沒有再被封鎖的名單當中

