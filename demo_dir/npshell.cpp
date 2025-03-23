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
    setenv("PATH", "bin:.", 1);

    int status;

    while (true) {
        Info myInfo = {false, {}, {}, {}};

        typePrompt(false);

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
                    cout << "[JID] " << pid << endl;
                } else {
                    waitpid(pid, &status, 0);
                }
            }
        }
    }

    return 0;
}


void typePrompt(bool showPath) {
    struct passwd *pw;
    char hostname[MAX_SIZE], cwd[MAX_SIZE];

    pw = getpwuid(getuid());
    gethostname(hostname, MAX_SIZE);
    getcwd(cwd, MAX_SIZE);
    if (showPath) {
        cout << pw->pw_name << "@" << hostname << ":~" << cwd << "$ ";
     } else {
        cout << "% ";
     }
}

int readCommand(Info &info) {
    vector<vector<string>> tempArgv;
    string command;

    if (!getline(cin, command)) {
        return -1;
    }

    istringstream iss(command);
    string token;
    int command_size = 0;
    
    tempArgv.push_back({});
    
    while (iss >> token) {
        if (token == "&") {
            info.bg = true;
        } else if (token == ">") {
            info.op.push_back(OUT_RD);
            info.numberpip.push_back(NOT_NUMBER_PIPE);
            command_size++;
            tempArgv.push_back({});
        } else if (token == "|") {
            info.op.push_back(PIPE);
            info.numberpip.push_back(NOT_NUMBER_PIPE);
            command_size++;
            tempArgv.push_back({});
        } else if (token[0] == '|') {
            info.op.push_back(PIPE);
            info.numberpip.push_back(stoi(token.substr(1, token.size() - 1)));
            command_size++;
            tempArgv.push_back({});
        } else {
            tempArgv[command_size].push_back(token);
        }
    }
    
    info.op.push_back(0);
    
    if (tempArgv[0].empty()) {
        return -1;
    }
    
    for (size_t i = 0; i < info.op.size(); ++i) {
        if (info.op[i] && tempArgv[i+1].empty()) {
            cerr << "Error: Syntax error near unexpected token 'newline'\n";
            return -1;
        }
    }

    info.argv = tempArgv;
    return 0;
}

void executeCommand(Info info) {
    for (size_t i = 0; i < info.argv.size(); ++i) {
        if (info.op[i] == PIPE) {
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
                for (auto &arg : info.argv[i]) {
                    args.push_back(arg.data());
                }
                args.push_back(nullptr);
    
                if (execvp(args[0], args.data()) == -1) {
                    cerr << "Unknown command: [" << args[0] << "]\n";
                    exit(1);
                }
            } else {
                int status;
                waitpid(pid, &status, 0);
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
            }
        } else {
            int fd;
            if (info.op[i] == OUT_RD) {
                fd = open(info.argv[i+1][0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (fd < 0) {
                    cerr << "Error: " << info.argv[i+1][0] << ": Failed to open or create file\n";
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
    
            vector<char*> args;
            for (auto &arg : info.argv[i]) {
                args.push_back(arg.data());
            }
            args.push_back(nullptr);
    
            if (execvp(args[0], args.data()) == -1) {
                cerr << "Unknown command: [" << args[0] << "]\n";
                exit(1);
            }
            break;
        }
    }

}