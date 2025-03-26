#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sys/wait.h>   // waitpid()
#include <fcntl.h>      // open()
#include <unistd.h>     // getcwd(), STDIN_FILENO, STDOUT_FILENO
#include <pwd.h>        // getpwuid()

constexpr int MAX_SIZE = 500;

constexpr int PIPE = 1;
constexpr int OUT_RD = 2;

constexpr int NOT_NUMBER_PIPE = 1;

using namespace std;

struct pipeStruct {
    int pipeStage;  // which command going to pipe
    int fd[2];      // pipe
};

struct Info {
    bool bg;                        // Execute in the background
    vector<int> op;                 // 0: single process, 1: pipline, 2: output redirection
    vector<int> numberpip;          // 0: Not number pipe, int: number pip int
    vector<vector<string>> argv;    // Arguments for first and second operand
    vector<int> pipeIndexList;      // pipeIndex, -1 if not pipe
};

void typePrompt(bool showPath);
int builtInCommand(Info info);
int readCommand(Info &info, const int totalCommandCount);
void executeCommand(Info info);
