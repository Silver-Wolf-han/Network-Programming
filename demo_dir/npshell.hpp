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

constexpr int NOT_NUMBER_PIPE = 0;

using namespace std;

struct pipeStruct {
    int pipeStage;  // numpipe handle, -1: not used, int: num pipe
    int fd[2];      // pipe
};

struct Info {
    bool bg;                        // Execute in the background
    vector<int> op;                 // 0: single process, 1: pipline, 2: output redirection
    vector<int> numberpip;          // 0: Not number pipe, int: number pip int
    vector<vector<string>> argv;    // Arguments for first and second operand
};

void typePrompt(bool showPath);
int builtInCommand(Info info);
int readCommand(Info &info);
void executeCommand(Info info, vector<pipeStruct> &pipeList);
