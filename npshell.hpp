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
constexpr int IN_RD = 2;
constexpr int OUT_RD = 3;

struct Info {
    bool bg;  // Execute in the background
    int op;   // 0: single process, 1: two-process pipeline, 2: input redirection, 3: output redirection
    std::vector<std::string> argv[2]; // Arguments for first and second operand
};


void typePrompt();
int readCommand(Info &info);
void executeCommand(Info info);
