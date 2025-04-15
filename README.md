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
以每個command的輸入創造pipe
:::
紀錄每個pipe的狀況
1. OutCommandIndex: 記錄輸出到哪個Command
2. fd[2]: 儲存pipe
3. relate_pids: 哪些process和這個pipe的輸入有關
4. startCommandType: 調整number pipe與oridary pipe順序

### Info
```=cpp
struct Info{
    bool bg;
    vector<int> op;
    vector<int> opOrder;
    vector<vector<string>> argv;
}
```
記錄每次輸入的Input Command以及這些Command的執行順序
1. bg: 是否背景執行
2. op: 每個command的pipe種類
    ```=hpp
    END_OF_COMMAND : 0
    PIPE           : 1
    NUM_PIPE       : 2
    NUM_PIPE_ERR   : 3
    OUT_RD         : 4
    ```
4. opOrder: 每個command應該pipe到哪個command
    ```=hpp
    NOT_PIPE        : 0
    NOT_NUMBER_PIPE : 1
    other(some n>0) : pipe to which command (n-th)
    ```
6. argv: command內容, 一個command存在一個`vector<string>`

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
註冊一個function來處理當process收到SIGCHLD signal。
透過flag`WNOHANG`來避免blocking，如果沒有child要結束就離開。

### typePrompt
```=cpp
input argument : showPath(bool)
return type    : void
```
顯示shell當中command line開始符號
傳入參數決定是否host name及current working directory

### readCommand
```=cpp
input argument : &info (struct Info) /* by reference*/
                 totalCommandCount (const int)
return type    : int
```
處理輸入argument並存到info當中
return總共有讀到多少commnad (output redirection記錄為1個)

過程處理Number Pipe與Ordiray pipe的counter問題-單行
```=bash
% removetag test.html |2 removetag  test.html | number
% number
```
需要將第一個`removetag` command pipe給下一行的`number`
處理方式：每次讀取到`'|'` (`op = PIPE`)就判斷同一行的的numbered pipe (`op = NUM_PIPE`)是否超過目前的ordiray pipe

### builtInComand
```=cpp
input argument : info (struct Info)
return type    : int
```
判斷是否built-in function(`printenv, setenv, exit, cd`)，是則執行
return是否為built-in function, `exit` return `-1`


### executeCommand
```=cpp
input argument : info (pipeStruct)
                 pipeMap (man<int,pipStruct>)
                 currentCommandStart (const int)
                 totalCommandCount   (const int)
return type    : void
```
透過info執行command
 
針對每個command:
1. 維護pipeMap
    如果該command pipe目標尚未創立接收的pipe，則創立pipe

2. `fork()`
   2-1. child process:連接pipe後執行command
   > 1. 如果連接至該pipe存在，則連接pipe(不存在表示存STDIN輸入)
   > 2. 判斷command輸出哪個 pipe 或 file 或 STDOUT 
   > 3. 用execvp執行command
       
   2-2. parent process:關閉pipe後等待child process 
   > 1. 收集相關的process pid
   > 2. 關閉相關的pipe
   > 3. 如果目前command是輸出到file or STDOUT，則等待相關的process執行
   > 4. 不是輸出到file or STDOUT可以先偷跑，大量資料可能被block住

3. `while (waitpid(-1, &status, WNOHANG)>0);`
    結束所有zombie processes

### main function
1. 設定環境變數 PATH="bin:."
2. print prompt
3. readCommand and get #com
4. update counter
5. 過程處理Number Pipe與Ordiray pipe的counter問題-多行
    ```=bash
    % removetag test.html |2
    % removetag test.html | number
    % number
    ```
    需要將第一個`removetag` command pipe給第三行的`number`
    處理方式：在執行下一個command之前，先掃描該行當中是否有Ordiray pipe(`'|'`)
    如果有的話，則檢查pipe input type是否為Number Pipe且是否超過目前執行行數
    若以上條件皆成立，則需要調整該條pipe的output至下一個(直接使用新的pipeMap來取代)
6. 若built-in command為`exit`，則跳至第九步
7. executecommand
8. 回到step 2
9. 結束

### demo part
八題抽一題

第六題 Ignore N line command (`%N`)
```=bash
% ls |5
% ls |2
% ls %2            # ignore 接下來兩行，目前的結果直接輸出
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
~~當初Demo我寫的是錯的，我把`ls |5`丟給cat，結果是一樣~~
1. op選項當中多加一個`IGNORE:5`
2. 讀到`%`的時候`info.op=IGNORE`, `info.opOrder=`後面的數字
3. `pipeMap`要跟著`IGNORE`往後推移 (和多行pipe補救放在一起) ~~demo時忘記做~~
   :::warning
   `ignore_idx++`這裡要記得**符合條件時:在要Ignore的期間**把要忽略的行數也要往後移動(`command A | command B`只算一個command)
   單行那邊不用補就是因為`command %N`, command會直接輸出，後面不會再接pipe
   :::
4. 目前的`command index <= ignore_idx`就直接不執行，跳下一個
5. 執行command時，若`op`為`IGNORE`，則把`opOrder`塞給`ignore_idx`(global變數)

第七題 Implement "+", there is no space between "+" (不小心拍到的)
```=bash
% ls |1+2
% noop
% noop
% number
    1 bin
    2 test.html
% # 剩下的沒拍到
```
就 在 填info.opOrder的時候，處理一下token讓他可以接"+"就好
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
以每個command的輸出創造pipe
:::
當有多個command輸入到同一command會覆蓋
:::warning
用merge pipe合併
:::
在小量輸出可以運作，遇到大量輸出如`manyblessings`會把buf塞爆
```=cpp
int mergePipe[2], buf[4096];
pipe(mergePipe);
while(read(inpipe, buf, sizeof(buf)) > 0) {
    write(outpipe, buf, sizeof(buf));
}
```
