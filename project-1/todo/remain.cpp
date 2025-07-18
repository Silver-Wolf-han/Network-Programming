#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include "npshell.hpp"

using namespace std;

int main() {
    signal(SIGCHLD, sigchld_handler);

    setenv("PATH", "bin:.", 1);

    int totalCommandCount = 0;

    vector<struct pipeStruct> pipeList;

    while (true) {
        Info myInfo = {false, {}, {}, {}};

        typePrompt(false);

        int commandNum = readCommand(myInfo, totalCommandCount);
        if (commandNum < 0) {
            continue;
        }
        
        
        int builtInFlag = builtInCommand(myInfo);

        int currentCommandStart = totalCommandCount;
        totalCommandCount += commandNum;

        for (size_t i = 0; i < myInfo.op.size(); ++i) {
            if (myInfo.op[i] != OUT_RD) {
                pipeList.push_back({myInfo.opOrder[i], {}, {}});
                if (myInfo.op[i] == PIPE || myInfo.op[i] == NUM_PIPE || myInfo.op[i] == NUM_PIPE_ERR) {
                    if (pipe(pipeList[pipeList.size()-1].fd) < 0) {
                        cerr << "Error: Unable to create pipe\n";
                        exit(1);
                    }
                }
            }
        }

        if (builtInFlag == -1) {
            break;
        } else if (!builtInFlag) {
            executeCommand(myInfo, pipeList, currentCommandStart, totalCommandCount);
        }
    }

    return 0;
}


void sigchld_handler(int signo) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
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

// return value: (1) 1, built-in function, continue read next command, (2) -1, exit or ^C (3) 0, not built-in
int builtInCommand(Info info) {

    // empty line or not single command
    if (info.argv[0].empty() || info.argv.size() != 1) {
        return 0;
    }

    // exit
    if (info.argv[0][0] == "exit") {
        return -1;
    }

    // setenv
    if (info.argv[0][0] == "setenv") {
        if (info.argv[0].size() == 3) {
            setenv(info.argv[0][1].c_str(), info.argv[0][2].c_str(), 1);
        } else {
            cerr << "command [setenv] lost parameter [var] or [value]" << endl;
        }
        return 1;
    }

    // printenv
    if (info.argv[0][0] == "printenv") {
        if (info.argv[0].size() == 2) {
            if (const char* env_p = getenv(info.argv[0][1].c_str())) {
                cout << env_p << endl;
            }
        }
        return 1;
    }

    // cd
    if (info.argv[0][0] == "cd") {
        if (chdir(info.argv[0][1].c_str()) < 0) {
            cerr << "Error: cd: " << info.argv[0][1] << ": No such file or directory\n";
        }
        return 1;
    }

    return 0;
}

int readCommand(Info &info, const int totalCommandCount) {
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
        } else if (token == ">" || token[0] == '|' || token[0] == '!') {
            info.op.push_back(
                (token == ">" ? OUT_RD: 
                    (token == "|" ? PIPE:
                        (token[0] == '|'? NUM_PIPE:
                            NUM_PIPE_ERR)
                    )
                )
            );
            info.opOrder.push_back(
                (token == ">" ? NOT_PIPE : totalCommandCount + command_size + 
                    (token.size() == 1 ? NOT_NUMBER_PIPE:stoi(token.substr(1, token.size() - 1))))
            );
            command_size++;
            tempArgv.push_back({});
        } else {
            tempArgv[command_size].push_back(token);
        }
        
        // Fix number_pipe output
        if (token == "|") {
            for (size_t i = 0; i < info.opOrder.size(); ++i) {
                if (info.op[i] == NUM_PIPE && (int)i + info.opOrder[i] < totalCommandCount + command_size + 1) {
                    info.opOrder[i]++;
                }
            }
        }
    }
    
    
    if (tempArgv[0].empty()) {
        return -1;
    }
    
    for (size_t i = 0; i < info.op.size(); ++i) {
        if (info.op[i] && tempArgv[i+1].empty()) {
            //cerr << "Error: Syntax error near unexpected token 'newline'\n";
            //return -1;
            tempArgv.erase(tempArgv.begin() + i + 1);
            break;
        }
    }
    
    info.argv = tempArgv;

    if (info.argv.size() != info.op.size()) {
        info.op.push_back(END_OF_COMMAND);
        info.opOrder.push_back(NOT_PIPE);
    }
    return (int)info.argv.size() - (info.op.size() > 1 && info.op[info.op.size()-2] == OUT_RD  ? 1:0);
}

void executeCommand(Info info, vector<struct pipeStruct> pipeList, const int currentCommandStart, const int totalCommandCount) {
    int status;

    //vector<pid_t> pids;

    for (size_t i = (size_t)currentCommandStart; i < (size_t)totalCommandCount; ++i) {
        /*
        int mergePipe[2];
        if (pipe(mergePipe) < 0) {
            cerr << "Error: Unable to create pipe\n";
            exit(1);
        }
        */

        size_t argvIndex = i - (size_t)currentCommandStart;
        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Error: Unable to fork" << endl;
        } else if (pid == 0) {
            
            
            
            // bool input_from_pipe = false;
            for (size_t j = 0; j < i; ++j) {
                if (pipeList[j].OutCommandIndex == (int)i) {
                    //input_from_pipe = true;
                    close(pipeList[j].fd[1]);
                    
                    /*
                    char buffer[4096];
                    ssize_t bytesRead;
                    ssize_t totalByteRead = 0;
                    while ((bytesRead = read(pipeList[j].fd[0], buffer, sizeof(buffer))) > 0) {
                        totalByteRead += totalByteRead;
                        write(mergePipe[1], buffer, bytesRead);
                    }
                    */
                    dup2(pipeList[j].fd[0], STDIN_FILENO);

                    close(pipeList[j].fd[0]);
                }
            }
            /*
            close(mergePipe[1]);
            if (input_from_pipe) {
                dup2(mergePipe[0], STDIN_FILENO);
            }
            close(mergePipe[0]);
            */
            if (info.op[argvIndex] == OUT_RD) {
                int fd;
                fd = open(info.argv[argvIndex+1][0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (fd < 0) {
                    cerr << "Error: " << info.argv[argvIndex+1][0] << ": Failed to open or create file\n";
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else if (info.op[argvIndex] == PIPE || info.op[argvIndex] == NUM_PIPE || info.op[argvIndex] == NUM_PIPE_ERR) {
                close(pipeList[i].fd[0]);
                if (info.op[argvIndex] == NUM_PIPE_ERR) {
                    dup2(pipeList[i].fd[1], STDERR_FILENO);
                }
                dup2(pipeList[i].fd[1], STDOUT_FILENO);
                close(pipeList[i].fd[1]);
            }
            
            vector<char*> args;
            for (auto &arg : info.argv[argvIndex]) {
                args.push_back(arg.data());
            }
            args.push_back(nullptr);

            if (execvp(args[0], args.data()) == -1) {
                cerr << "Unknown command: [" << args[0] << "]." << endl;
                exit(1);
            }
        } else {
            // pids.push_back(pid);
            for (size_t j = 0; j < i; ++j) {
                if (pipeList[j].OutCommandIndex == (int)i) {
                    for (size_t k = 0; k < pipeList[j].relate_pids.size(); ++k) {
                        pipeList[i].relate_pids.push_back(pipeList[j].relate_pids[k]);
                    }
                }
            }
            pipeList[i].relate_pids.push_back(pid);

            //close(mergePipe[0]);close(mergePipe[1]);
            

            for (size_t j = 0; j <= i; ++j) {
                if (pipeList[j].OutCommandIndex >= currentCommandStart && pipeList[j].OutCommandIndex <= (int)i && pipeList[j].OutCommandIndex != -1) {
                    close(pipeList[j].fd[0]);
                    close(pipeList[j].fd[1]);
                }
            }
            
            //waitpid(pid, &status, 0);

            if (info.op[argvIndex] == END_OF_COMMAND || info.op[argvIndex] == OUT_RD) {
                for (pid_t pid_i: pipeList[i].relate_pids) {
                    waitpid(pid_i, &status, 0);
                }
            }
        }
        
    }
    /*
    for (pid_t pid: pids) {
        waitpid(pid, &status, 0);
    }
    */
    while (waitpid(-1, &status, WNOHANG) > 0);
}