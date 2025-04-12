#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include "np_sample.hpp"

using namespace std;


//size_t var = 0;

/*
int main(int argc, char* argv[]) {
    npshellInit();
    npshellLoop();
    return 0;
}
*/

void sigchld_handler(int signo) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void npshellInit() {
    signal(SIGCHLD, sigchld_handler);
    chdir("working_directory");
    setenv("PATH", "bin:.", 1);
}

void npshellLoop() {
    int totalCommandCount = 0;

    map<int, struct pipeStruct>  pipeMap;

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
            if (myInfo.op[i] == PIPE) {
                map<int, struct pipeStruct> tempMap;
                for (auto [key, value]:pipeMap) {
                    if (value.OutCommandIndex > currentCommandStart + (int)i) {
                        value.OutCommandIndex++;
                        tempMap[key+1] = value;
                    } else {
                        tempMap[key] = value;
                    }
                }
                pipeMap = tempMap;
            }
            
        }

        if (builtInFlag == -1) {
            break;
        } else if (!builtInFlag) {
            executeCommand(myInfo, pipeMap, currentCommandStart, totalCommandCount);
        }
    }
}

void npshell_handle_one_line(map<int, UserInfo>& User_Info_Map, const int user_idx, bool *exit) {

    chdir("working_directory");
    
    Info myInfo = {false, {}, {}, {}};

    int commandNum = readCommand(myInfo, User_Info_Map[user_idx].totalCommandCount);
    if (commandNum < 0) {
        return;
    }
    
    int builtInFlag = builtInCommand(myInfo);

    int currentCommandStart = User_Info_Map[user_idx].totalCommandCount;
    User_Info_Map[user_idx].totalCommandCount += commandNum;
    
    for (size_t i = 0; i < myInfo.op.size(); ++i) {
        if (myInfo.op[i] == PIPE) {
            map<int, struct pipeStruct> tempMap;
            for (auto [key, value]:User_Info_Map[user_idx].pipeMap) {
                if (value.OutCommandIndex > currentCommandStart + (int)i) {
                    value.OutCommandIndex++;
                    tempMap[key+1] = value;
                } else {
                    tempMap[key] = value;
                }
            }
            User_Info_Map[user_idx].pipeMap = tempMap;
        }
        
    }

    if (builtInFlag == -1) {
        *exit = true;
    } else if (builtInFlag == 1 && myInfo.argv[0][0] == "who") {

    } else if (builtInFlag == 1 && myInfo.argv[0][0] == "tell") {

    } else if (builtInFlag == 1 && myInfo.argv[0][0] == "yell") {

    } else if (builtInFlag == 1 && myInfo.argv[0][0] == "name") {

    } else if (!builtInFlag) {
        executeCommand(myInfo, User_Info_Map[user_idx].pipeMap, currentCommandStart, User_Info_Map[user_idx].totalCommandCount);
        typePrompt(false);
    }

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
    cout.flush();
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

    // who
    if (info.argv[0][0] == "who") {
        return 1;
    }

    // tell
    if (info.argv[0][0] == "tell") {
        return 1;
    }

    // yell
    if (info.argv[0][0] == "yell") {
        return 1;
    }

    // name
    if (info.argv[0][0] == "name") {
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
        } /*else if (token[0] == '%') {
            info.op.push_back(IGNORE);
            info.opOrder.push_back(totalCommandCount + command_size + stoi(token.substr(1, token.size() - 1)));
            command_size++;
            tempArgv.push_back({});
        } */else {
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

void executeCommand(Info info, map<int, struct pipeStruct>& pipeMap, const int currentCommandStart, const int totalCommandCount) {
    int status;

    for (size_t i = (size_t)currentCommandStart; i < (size_t)totalCommandCount; ++i) {
        /*
        if (var != 0 && i <= var) {
            continue;
        }
        */

        size_t argvIndex = i - (size_t)currentCommandStart;
        /*
        if (info.op[argvIndex] == IGNORE) {
            var = info.opOrder[argvIndex];
        }
        */
        if (info.op[argvIndex] != OUT_RD) {
            if ((info.op[argvIndex] == PIPE || info.op[argvIndex] == NUM_PIPE || info.op[argvIndex] == NUM_PIPE_ERR) && 
            pipeMap.find(info.opOrder[argvIndex]) == pipeMap.end()) {
                pipeMap[info.opOrder[argvIndex]] = {info.opOrder[argvIndex], {}, {}, info.op[argvIndex]};
                if (pipe(pipeMap[info.opOrder[argvIndex]].fd) < 0) {
                    cerr << "Error: Unable to create pipe" << endl;
                    exit(1);
                }
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Error: Unable to fork" << endl;
        } else if (pid == 0) {
            if (pipeMap.find((int)i) != pipeMap.end()) {
                close(pipeMap[(int)i].fd[1]);
                dup2(pipeMap[(int)i].fd[0], STDIN_FILENO);
                close(pipeMap[(int)i].fd[0]);
            }
            
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
                close(pipeMap[info.opOrder[argvIndex]].fd[0]);
                if (info.op[argvIndex] == NUM_PIPE_ERR) {
                    dup2(pipeMap[info.opOrder[argvIndex]].fd[1], STDERR_FILENO);
                }
                dup2(pipeMap[info.opOrder[argvIndex]].fd[1], STDOUT_FILENO);
                close(pipeMap[info.opOrder[argvIndex]].fd[1]);
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
            if (pipeMap.find((int)i) != pipeMap.end()) {
                pipeMap[(int)i].relate_pids.push_back(pid);
            }

            if (pipeMap.find((int)i) != pipeMap.end()) {
                close(pipeMap[(int)i].fd[0]);
                close(pipeMap[(int)i].fd[1]);
            }
            
            if ((info.op[argvIndex] == END_OF_COMMAND || info.op[argvIndex] == OUT_RD) && pipeMap.find((int)i) != pipeMap.end()) {
                for (pid_t pid_i: pipeMap[(int)i].relate_pids) {
                    waitpid(pid_i, &status, 0);
                }
            } else if (info.op[argvIndex] == END_OF_COMMAND || info.op[argvIndex] == OUT_RD /*|| info.op[argvIndex] == IGNORE*/) {
                waitpid(pid, &status, 0);
            }
        }
        
    }
    while (waitpid(-1, &status, WNOHANG) > 0);
}
