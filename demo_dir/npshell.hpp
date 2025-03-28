#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sys/wait.h>   // waitpid()
#include <fcntl.h>      // open()
#include <unistd.h>     // getcwd(), STDIN_FILENO, STDOUT_FILENO
#include <pwd.h>        // getpwuid()
#include <signal.h>     // signal()

constexpr int MAX_SIZE = 500;

constexpr int END_OF_COMMAND = 0;   // op
constexpr int PIPE = 1;
constexpr int PIPE_ERROR = 2;
constexpr int OUT_RD = 3;

constexpr int NOT_PIPE = -1;        // opOrder
constexpr int NOT_NUMBER_PIPE = 1;

using namespace std;

struct pipeStruct {
    int OutCommandIndex;    // pipe output to which command
    int fd[2];              // pipe
};

struct Info {
    bool bg;                        // Execute in the background
    vector<int> op;                 // 0: single process, 1: pipline, 2: output redirection
    vector<int> opOrder;            // output to which command
    vector<vector<string>> argv;    // Arguments for first and second operand
    vector<int> pipeIndexList;      // pipeIndex, -1 if not pipe
};

void sigchld_handler(int signo);
void typePrompt(bool showPath);
int builtInCommand(Info info);
int readCommand(Info &info, const int totalCommandCount);
void executeCommand(Info info, vector<struct pipeStruct> pipeList, const int currentCommandStart, const int totalCommandCount);
//void executeCommand(Info info);
