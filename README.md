# project-2-Silver-Wolf-han

# Part 1 Concurent Connection-Oriented Server

## header (np_simple.hpp)
啥事沒幹 `include "npshell.hpp"` 和 `#define MAX_CLIENT 30` 而已

## source code (np_simple.hpp)

### main
處理 port (從argv裡面拿，沒有就預設7001)

### concurentConnectionOrientServer
```=cpp
input argument : port(int)
return type    : void
```
就很標準的過程，為了handle多個Client就fork()一個出來
Parent繼續等accept()，Child就處理一個物品
```
Server:
create socket -> set socket opt -> binding -> listen -> accept -> fork()
                                                         ∧ | ∧    / \
                                                        /  |  \  ∨   ∨
                                                       /   | parent child
                                                      /    ∨          /
Client:                                            send  getfd <----
```
`SO_REUSEADDR`:[How do SO_REUSEADDR and SO_REUSEPORT differ?](https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ)
```=cpp
// create socket
server_fd = socket(AF_INET, SOCK_STREAM, 0);
// set socket opt：這步驟是為了可以讓同一個IP的可以連線
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
// server address setting
server_addr設定3行 包含設定port
memset(&(server_addr.sin_zero), 0, 8);
// binding
bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
// listen
listen(server_fd, MAX_CLIENT);
// zombie process
把sigchld_handler拿出來用
// accept
while (true) {
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &sin_size);
    pid_t pid = fork();
    if (fork() == 0) {
        // child
        close(server_fd);
        
        // 從client_fd讀輸入，把輸出丟到client_fd
        dup2(client_fd, STDIN_FILENO);
        dup2(client_fd, STDOUT_FILENO);
        dup2(client_fd, STDERR_FILENO);
        
        // 處理npshell
        npshellInit();
        npshellLoop();
        
        close(client_fd);
        exit(0);
    } else {
        // parent
        close(client_fd);
    }
}
```

以下兩個是放在`npshell.cpp`當中，不過基本上就是專屬 Part 1，而且和原本的npshell也很像，所以就放在這裡

### npshellInit
```=cpp
input argument : None
return type    : void
```
就註冊sigchld_hander，進working_directory，設定`PATH="bin:."`而已

### npshellLoop
```=cpp
input argument : None
return type    : void
```
和 Project 1 基本一樣，只有和 Part 2 共用某些function時多塞一些`NULL`進去而已
:::info
所以 Part 2 提到和 Project 1 有關的部分會主要強調為什麼要多寫一個function
:::

# Part 2

## header (np_single_proc.hpp)
啥事沒幹
```=cpp
#include "npshell.hpp"
constexpr int MAX_CLIENT = 40;
constexpr int FD_UNDEFINED = -1;
```

## source code (np_single_proc.cpp)

### main
和 Part 1 一樣

### singleProcessConcurrentServer
```
input argument : port (int)
return type    : void
```
到 accept loop 以前和 Part 1 一模一樣，就創 socket bind listen
但因為後面是`single process`，**所以這裡要針對不同的clinet連進來的fd進行處理**

[freeBSD FD man page](https://man.freebsd.org/cgi/man.cgi?query=FD_SET&apropos=0&sektion=3&manpath=FreeBSD+7-current&format=html)
```=cpp
/* 下面都是 <sys/select>下面定義好的 mirco*/
FD_ZERO(&fdset);        // 清空fd_set*
FD_SET(int fd, &fdset); // 把fd加進去fdset
FD_ISSET(fd, &fdset);   // fd有沒有在fdset裡
FD_CLR(fd, &fdset);     // 殺掉fd
```

```=cpp
// 用來儲存所有client fd的set
fd_set all_fd_set;
// 用來存「Accpet新的」之前的fdset
fd_set rset;
```

1. select 看看有沒有人有要做事
    `select(max_fd_history+1, &rset, NULL, NULL, (struct timeval *)0)`會回傳有幾個要做事，後面迴圈要根據回傳值來決定要處理幾次
2. accept 看有沒有新的人要連線
    如果有就幫init做一做(存進User_Info_Map)
    2-1. **Environment Variable**要另外存起來，等到要執行時每個人的拿出來蓋掉
    2-2. 做完的先加進`fd_all_set`，但不要加入`rset`
    2-3. 如果要處理的人(return of `select()`)已經處理完了 跳到1.
3. 接著跑過全部的client_fd_table來看誰要做事
    3-1. **要怎麼區分這個Client是剛剛連進來的，還是之前連進來過想要做事?** 用`rset`來區分，在`rset`裡面的表示不是之前連進來的
    3-2. Environment Variable 設一設
    3-3. 把輸出輸入交給 client
    3-4. 處理他丟進來的command (只會有一行)
        3-4-1. 如果command包含exit，就把這個人登出，相關資料刪一刪
    3-5. 判斷要處理完的人處理完沒(by return of `select()`
        3-5-1. 還沒就回到 3.
        3-5-2. 處理完了就回到1.




# Functional Handler (Extension From Project 1)

## header (npshell.hpp)

`pipeStruct`, `Info`和 Project 1 裡面的是一樣的

### UserInfo
```=cpp
struct UserInfo {
    char IPAddress[INET_ADDRSTRLEN];
    int port;
    string UserName;
    map<string, string> EnvVar;
    int totalCommandCount;
    map<int, struct pipeStruct> pipeMap;
};
```
負責紀錄每個使用者(登入的人)的狀態
1. IPAddress: 存IP(`127.0.0.1`, `140.113.190.87`之類的)
2. Port: 存Port(同一個IP可以開不同的port來登入`1024~65536`)
3. UserName: 使用者名稱, default = `"(no name)"`
4. EnvVar: 每個使用者要自己存環境變數(最主要是`PATH`，這個使用者執行到那些檔案)
5. totalCommandCount: 和 Project 1 一樣的，每個使用者要自己存(不能和其他的number pipe混用)
6. pipeMap: 同上

## source code (npshell.cpp)

### User Pipe handler
global variable `map<pair<int,int>, pipeStruct> UserPipes`
1. Key: `UserPipes.first`是一個`pair<int,int>`，表示這個pipe是從哪個 User ID(`UserPipes.first.first`) 到哪個 User ID(`User.first.second`) 
   :::warning
   -1 表示 User 不存在，不存在的話 Pipe 仍要創立，就從`/dev/null`讀取或丟到`/dev/null`裡面
   :::
2. Value: 就 PipeStruct 負責存 Pipe 的

### npshell_handle_one_line
```=cpp
input argument : User_Info_Map (map<int, UserInfo>&)
                 user_idx (const int)
                 exit (bool*)
                 client_fd_table (const int* const)
return type    : void
```
處理 single process (Part 2) 的 npshell，只有一行是因為
~~0. 自己測試時先進入`"working_dirctory"`~~
(環境變數在傳進這個function前就處理好了)
基本上和npshell loop或project 1的部分很像，不一樣的地方有
1. built-in function
   會重寫主要是因為`setenv`和`printenv`，在Concurrent Connect-Oriented裡面，可以直接交給`fork()`出來的process來handle，可是這裡不能call`fork()`，要自己man，所以就重寫一個
2. handle `exit`
   2-1. 之前`exit()`直接break離開loop就好，不過這裡只有一行，所以設定input argument有一個exit來讓`singleprocessserver()`知道要離開
   2-2. 離開之前要把UserPipes清乾淨
3. call `typePrompt()`的位置
   ~~cout是buffer IO，直接粗暴地加上`cout.flush()`~~
   改成輸出完welcome msg後、command執行完後輸出
   

### broadcast
```=cpp
input argument : msg (string)
                 client_fd_table (const int* const)
                 User_Info_Map (const map<int, UserInfo>)
return type    : void
```
就 換client fd然後輸出 (**broadcast不會把輸出切回去，所以call完後要自己切回去**)
```=cpp
for (auto user : User_Info_Map) {
    if (client_fd_table[user.first] != -1) {
        dup2Client(client_fd_table[user.first]);
        cout << msg << endl;
    }
}
```

### dup2Client
同樣不會切回來
```=cpp
void dup2Client(int fd) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}
```

### builtInCommand_com_handle
為了Environment Variable重寫一個function，一個process直接處理，另一個就要存在`User_Info_Map`
其他大致上相同(`block`和`unblock`放在demo part好了)
多的function (`who`, `name`, `tell`, `yell`)就依照spec用`User_Info_Map`來處理

### ReConstructCommand
```=cpp
input agrument : info (const Info)
               : currentCommandStart (const int)
return type    : string
```
因為UserPipes broadcast需要用到完整的command，阿Project 1就直接切掉了，只好重組回來
用`currentCommnadStart`來組出原本的command當中的數字是多少

### executeCommand
```=cpp
input argument : info (Info)
               : pipeMap (map<int, struct pipeStruct>&)   // 每個User自己保存
               : currentCommandStart (const int)          // 每個User自己保存
               : totalCommandCOunt (const int)            // 每個User自己保存
               : ignore_idx (size_t*)                     // 每個User自己保存
               : user_idx (const int)
               : User_Info_Map (const map<int, UserInfo>) // 每個User自己保存
               : client_fd_table (const int* const)
return type    : void
```
理論上變最多東西的function，傳那麼多東西進來是因為每個User都有自己的要保存
(~~其實只要傳User_Info_Map進來就好，可是我寫到最後決定把它傳進來，就不想動它了~~)

新增的東西都是和以下這個globla variable有關
```=cpp
map<pair<int,int>, struct pipeStruct> UserPipes;
```
`UserPipes.first`是一個`pair<int,int>`，表示後面是一個從User幾到User幾的pipe
    -1，表示不存在，**還是要繼續執行**，執行的結果就從`/dev/null`裡面拿 (by TA Slide)
`UserPipes.second`是一個`struct pipeStuct>`就只是存pipe而已(只有用到裡面的fd)

1. 在執行前要判斷是否有從其他User收Pipe，或是要Pipe到其他User
   1-1. 先去從`info.argv[argvIndx]`裡面撈撈看有沒有`<`或`>`字元 (在這裡readCommand就不用改了)，撈到後**不能刪掉**，要`fork()`完child要執行前才可以殺掉，parent不能殺，因為其他地方輸出要用到
   1-2. 依照spec規則判斷pipe是否正常，**同時判斷是否需要創造pipe**，可能不正常的也要有pipe
       1-2-1. 接收自其他User的pipe
           1-2-1-1. 不正常: 要創造pipe `{-1, user_idx}:pipe`
           1-2-1-2. 正常: 不用創造pipe，broadcast收到了就好
       1-2-2. 要送東西到其他User
           1-2-2-1. 不正常: 要創造pipe `{user_idx, -1}:pipe`
           1-2-2-2. 正常: 還是要創造pipe `{user_idx, to_user_idx}:pipe`
           
2. `fork()`之後，child執行command之前
    2-1. 把`<`和`>`移調才可以執行
    2-2. 如果UserPipe輸入有找到對應的Pair，把輸入接過來
        -1就用`open("/dev/null", O_RDOONLY, 0)`
    2-3. 如果UserPipe輸出有找到對影的Pair，把輸出接過去
        -1就用`open("/dev/null", O_RDWR, 0)`

3. `fork()`之後，parent把該關掉的pipe關一關
    3-1. 如果pipe.first有是-1的，表示剛剛child一定要有執行到(不然會出事)，直接從pipe趕出去

剩下的應該都一樣 吧


### demo part
自己偷跑寫`block`和`unblock` (~~寫了四個小時..十分鐘怎麼可能寫得完~~)
自訂spec
1. 若A 下`block B`
   1-1. B 無法透過`tell A msg` 和 command `command >A`來傳遞
   1-2. 若A此時下`commad <B`則會當作pipe不存在
   1-3. A還是可以透過`tell B msg`和`command >B`來送東西給B
   1-4. B下`yell`還是可以收到
       1-4-1 如果要做出收不到的版本，應該是改`broadcast function`要把誰call的傳進去讓他判斷有有沒有被block住
   1-5. 如果A又下了一次`block B`則會輸出Error
   

2. 若A在沒有下`block B`的情況下`unblock B`輸出Error

Example
```=bash
# User 1                       # User 2
% block 2            
                               % tell 1 test
                               *** Error: been block ***
                               % removetag test.html >1
                               *** Error: been block ***
% number <1
*** Error: pipe not exist. ***
% unblock 2
                               % tell 1 test
% *** 2 tell : test ***
                               % removetag test.html >1
*** receive command from 2 ***
% cat <2
...Some output...
```

怎麼做?
1. built-in function 多加`block`和`unblock`
   1-1. `UserInfo`多加一個`vector<int> who_block_me`，紀錄誰block我
        (想記錄誰block我的原因是這樣我執行command的時候才可以很方便地知道要不要丟到`/dev/null`
        只是這樣要block broadcast還是得反過來)
   1-2. `find(User_Info_Map[target].who_block_me(), user_idx) == end?`
        去別人的list找看看有沒有我
   1-3. 依照spec輸出對應的錯誤或加東西刪東西

２. 執行`command >n`之前先去`find(User_Info_Map[user_idx], n) == end?`看看有沒有block，有被block就當作錯誤處理