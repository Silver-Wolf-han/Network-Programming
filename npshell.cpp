#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include "npshell.hpp"

using namespace std;

int main() {
    int status;

    while (true) {
        Info myInfo = {false, 0, {{}, {}}};

        typePrompt();

        if (readCommand(myInfo) < 0) {
            continue;
        }

        if (!myInfo.argv[0].empty() && myInfo.argv[0][0] == "exit") {
            break;
        } else if (!myInfo.argv[0].empty() && myInfo.argv[0][0] == "cd") {
            if (chdir(myInfo.argv[0][1].c_str()) < 0) {
                cerr << "Error: cd: " << myInfo.argv[0][1] << ": No such file or directory\n";
            }
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                cerr << "Error: Unable to fork\n";
                exit(1);
            } else if (pid == 0) {
                executeCommand(myInfo);
            } else {
                if (myInfo.bg) {
                    cout << "[JID] " << pid << std::endl;
                } else {
                    waitpid(pid, &status, 0);
                }
            }
        }
    }

    return 0;
}

void typePrompt() {
    struct passwd *pw;
    char hostname[MAX_SIZE], cwd[MAX_SIZE];

    pw = getpwuid(getuid());
    gethostname(hostname, MAX_SIZE);
    getcwd(cwd, MAX_SIZE);

    cout << pw->pw_name << "@" << hostname << ":~" << cwd << "$ ";
}

int readCommand(Info &info) {
    bool firstPrgm = true;
    vector<string> tempArgv[2];
    string command;

    if (!getline(cin, command)) {
        return -1;
    }

    istringstream iss(command);
    string token;
    
    while (iss >> token) {
        if (token == "&") {
            info.bg = true;
        } else if (token == ">") {
            info.op = OUT_RD;
            firstPrgm = false;
        } else if (token == "<") {
            info.op = IN_RD;
            firstPrgm = false;
        } else if (token == "|") {
            info.op = PIPE;
            firstPrgm = false;
        } else if (firstPrgm) {
            tempArgv[0].push_back(token);
        } else {
            tempArgv[1].push_back(token);
        }
    }

    if (tempArgv[0].empty() && info.op == 0 && tempArgv[1].empty()) {
        return -1;
    }

    if (info.op && tempArgv[1].empty()) {
        cerr << "Error: Syntax error near unexpected token 'newline'\n";
        return -1;
    }

    info.argv[0] = tempArgv[0];
    info.argv[1] = tempArgv[1];

    return 0;
}

void executeCommand(Info info) {
    if (info.op == PIPE) {
        int fd[2];

        if (pipe(fd) < 0) {
            cerr << "Error: Unable to create pipe\n";
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Error: Unable to fork\n";
            exit(1);
        } else if (pid == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            vector<char*> args;
            for (auto &arg : info.argv[0]) {
                args.push_back(arg.data());
            }
            args.push_back(nullptr);

            if (execvp(args[0], args.data()) == -1) {
                cerr << "Error: \"" << args[0] << "\" command not found\n";
                exit(1);
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);

            vector<char*> args;
            for (auto &arg : info.argv[1]) {
                args.push_back(arg.data());
            }
            args.push_back(nullptr);

            if (execvp(args[0], args.data()) == -1) {
                cerr << "Error: \"" << args[0] << "\" command not found\n";
                exit(1);
            }
        }
    } else {
        int fd;
        if (info.op == IN_RD) {
            fd = open(info.argv[1][0].c_str(), O_RDONLY);
            if (fd < 0) {
                cerr << "Error: " << info.argv[1][0] << ": No such file or directory\n";
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else if (info.op == OUT_RD) {
            fd = open(info.argv[1][0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd < 0) {
                cerr << "Error: " << info.argv[1][0] << ": Failed to open or create file\n";
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        vector<char*> args;
        for (auto &arg : info.argv[0]) {
            args.push_back(arg.data());
        }
        args.push_back(nullptr);

        if (execvp(args[0], args.data()) == -1) {
            cerr << "Error: \"" << args[0] << "\" command not found\n";
            exit(1);
        }
    }
}
