#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sys/wait.h>   // waitpid()
#include <fcntl.h>      // open()
#include <unistd.h>     // getcwd(), STDIN_FILENO, STDOUT_FILENO
#include <pwd.h>        // getpwuid()
#include <signal.h>     // signal()

constexpr int MAX_SIZE = 1000;

constexpr int END_OF_COMMAND = 0;   // op
constexpr int PIPE = 1;
constexpr int NUM_PIPE = 2;
constexpr int NUM_PIPE_ERR = 3;
constexpr int OUT_RD = 4;
// constexpr int IGNORE = 5;

constexpr int NOT_PIPE = -1;        // opOrder
constexpr int NOT_NUMBER_PIPE = 1;

using namespace std;

struct pipeStruct {
    int OutCommandIndex;        // pipe output to which command
    int fd[2];                  // pipe
    vector<pid_t> relate_pids;  // input proc and previous proc
    int startCommandType;       // record the start command type
};

struct Info {
    bool bg;                        // Execute in the background
    vector<int> op;                 // 0: single process, 1: pipline, 2: output redirection
    vector<int> opOrder;            // output to which command
    vector<vector<string>> argv;    // Arguments for first and second operand
};

struct UserInfo {
    char IPAddress[INET_ADDRSTRLEN];
    int port;
    string UserName;
    map<string, string> EnvVar;
    int totalCommandCount;
    map<int, struct pipeStruct> pipeMap;
};

void npshellInit();
void npshellLoop();
void npshell_handle_one_line(map<int, UserInfo>& User_Info_Map, const int user_idx, bool *exit);
void sigchld_handler(int signo);
void typePrompt(bool showPath);
int builtInCommand(Info info);
int readCommand(Info &info, const int totalCommandCount);
void executeCommand(Info info, map<int, struct pipeStruct>& pipeMap, const int currentCommandStart, const int totalCommandCount);
