#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sys/wait.h>   // waitpid()
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>      // open()
#include <unistd.h>     // getcwd(), STDIN_FILENO, STDOUT_FILENO
#include <pwd.h>        // getpwuid()
#include <signal.h>     // signal()
#include <string>
#include <algorithm>    // find()
#include <netinet/in.h> // **WAIT**
#include <arpa/inet.h>  // **WAIT**
#include <cstring>      // memset
#include <sstream>

constexpr int MAX_SIZE = 1000;

constexpr int END_OF_COMMAND = 0;   // op
constexpr int PIPE = 1;
constexpr int NUM_PIPE = 2;
constexpr int NUM_PIPE_ERR = 3;
constexpr int OUT_RD = 4;
constexpr int IGNORE = 5;

constexpr int NOT_PIPE = -1;        // opOrder
constexpr int NOT_NUMBER_PIPE = 1;

constexpr int MAX_CLIENT = 40;
constexpr int MAX_COMMAND_SIZE = 15000;
constexpr int NAME_SIZE = 20;

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
    int client_fd;
    pid_t pid;
    char IPAddress[INET_ADDRSTRLEN];
    int port;
    char UserName[NAME_SIZE];
    // int totalCommandCount;
    // size_t ignore_idx;
    // map<int, struct pipeStruct> pipeMap;
    // vector<int> who_block_me;
};

void sigchld_handler(int signo);
void sendMsg(int signo);
void release_share_memory(int signo);
void npshellInit();
void npshellLoop(const size_t user_idx);
void dup2Client(int fd);
void sigchld_handler(int signo);
void typePrompt(bool showPath);
int builtInCommand(Info info, const size_t user_idx);
int readCommand(Info &info, const int totalCommandCount);
string ReConstructCommand(const Info info, const int currentCommandStart);
void executeCommand(Info info, map<int, struct pipeStruct>& pipeMap, const int currentCommandStart,
                     const int totalCommandCount, size_t *ignore_idx);
void broadcast(string msg);
void dup2Client(int fd);


void concurentConnectionOrientedServer(int port);
