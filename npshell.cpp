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


map<pair<int,int>, struct pipeStruct> UserPipes;

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
            executeCommand(myInfo, pipeMap, currentCommandStart, totalCommandCount, 0, {}, NULL);
        }
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

void npshell_handle_one_line(map<int, UserInfo>& User_Info_Map, const int user_idx, 
                            bool *exit, const int* const client_fd_table) {

    chdir("working_directory");
    
    Info myInfo = {false, {}, {}, {}};

    int commandNum = readCommand(myInfo, User_Info_Map[user_idx].totalCommandCount);
    if (commandNum < 0) {
        return;
    }
    
    int builtInFlag = builtInCommand_com_handle(myInfo, User_Info_Map, user_idx, client_fd_table);

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
        return;
    } else if (!builtInFlag) {
        executeCommand(myInfo, User_Info_Map[user_idx].pipeMap, currentCommandStart, 
            User_Info_Map[user_idx].totalCommandCount, user_idx, User_Info_Map, client_fd_table);
    }
    typePrompt(false);
}

void dup2Client(int fd) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}

void broadcast(string msg, const int* const client_fd_table, const map<int, UserInfo> User_Info_Map) {
    for (auto user : User_Info_Map) {
        if (client_fd_table[user.first] != -1) {
            dup2Client(client_fd_table[user.first]);
            cout << msg << endl;
        }
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

int builtInCommand_com_handle(Info info, map<int, UserInfo>& User_Info_Map, const int user_idx, 
                            const int* const client_fd_table) {

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
            User_Info_Map[user_idx].EnvVar[info.argv[0][1]] = info.argv[0][2];
        } else {
            cerr << "command [setenv] lost parameter [var] or [value]" << endl;
        }
        return 1;
    }

    // printenv
    if (info.argv[0][0] == "printenv") {
        if (info.argv[0].size() == 2 && 
            User_Info_Map[user_idx].EnvVar.find(info.argv[0][1]) != User_Info_Map[user_idx].EnvVar.end()) {
            cout << User_Info_Map[user_idx].EnvVar[info.argv[0][1]] << endl;
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
        cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>" << endl;
        for (auto user : User_Info_Map) {
            if (client_fd_table[user.first] != -1) {
                cout << user.first << "\t" << user.second.UserName << "\t" << user.second.IPAddress 
                     << ":" << user.second.port;
                if (user_idx == user.first) {
                    cout << "\t" << "<-me";
                }
                cout << endl;
            }
        }
        return 1;
    }

    // tell
    if (info.argv[0][0] == "tell") {
        int receiver = stoi(info.argv[0][1]);
        if (User_Info_Map.find(receiver) == User_Info_Map.end() || client_fd_table[receiver] == -1) {
            cerr << "*** Error: user #" << receiver << " does not exist yet. ***" << endl;
        } else {
            dup2Client(client_fd_table[receiver]);
            cout << "*** " << User_Info_Map[user_idx].UserName << " told you ***:";
            for (size_t i = 2; i < info.argv[0].size(); ++i) {
                cout << " " << info.argv[0][i];
            }
            cout << endl;
            dup2Client(client_fd_table[user_idx]);
        }
        return 1;
    }

    // yell
    if (info.argv[0][0] == "yell") {
        string yellMsg = "*** " + User_Info_Map[user_idx].UserName + " yelled ***:";
        for (size_t i = 1; i < info.argv[0].size(); ++i) {
            yellMsg += (" " + info.argv[0][i]);
        }
        broadcast(yellMsg, client_fd_table, User_Info_Map);
        dup2Client(client_fd_table[user_idx]);
        return 1;
    }

    // name
    if (info.argv[0][0] == "name") {
        bool same_name_exist = false;
        for (auto user : User_Info_Map) {
            if (client_fd_table[user.first] != -1) {
                if (user.second.UserName == info.argv[0][1]) {
                    cout << "*** User '" << user.second.UserName << "' already exists. ***" << endl;
                    same_name_exist = true;
                    break;
                }
            }
        }
        if (!same_name_exist) {
            User_Info_Map[user_idx].UserName = info.argv[0][1];
            string changeNameMsg = "*** User from " + string(User_Info_Map[user_idx].IPAddress) + ":"
                                 + to_string(User_Info_Map[user_idx].port) + " is named '" 
                                 + User_Info_Map[user_idx].UserName + "'. ***";
            broadcast(changeNameMsg, client_fd_table, User_Info_Map);
            dup2Client(client_fd_table[user_idx]);
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
    
    bool is_tell_or_yell = false;

    while (iss >> token) {
        if (command_size == 0 && (token == "yell" || token == "tell")) {
            is_tell_or_yell = true;
        }

        if (token == "&") {
            info.bg = true;
        } else if (!is_tell_or_yell && (token == ">" || token[0] == '|' || token[0] == '!')) {
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
        if (!is_tell_or_yell && token == "|") {
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

void executeCommand(Info info, map<int, struct pipeStruct>& pipeMap, const int currentCommandStart, 
                    const int totalCommandCount, const int user_idx, const map<int, UserInfo> User_Info_Map, 
                    const int* const client_fd_table) {
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

        int from_user_pipe = -1, to_user_pipe = -1;
        size_t from_token_idx = 0, to_token_idx = 0;
        for (size_t i = 0; i < info.argv[argvIndex].size(); ++i) {
            if (info.argv[argvIndex][i].size() != 1 && info.argv[argvIndex][i][0] == '<') {
                from_user_pipe = stoi(info.argv[argvIndex][i].substr(1, info.argv[argvIndex][i].size() - 1));
                from_token_idx = i;
            }

            if (info.argv[argvIndex][i].size() != 1 && info.argv[argvIndex][i][0] == '<') {
                to_user_pipe = stoi(info.argv[argvIndex][i].substr(1, info.argv[argvIndex][i].size() - 1));
                to_token_idx = i;
            }
        }

        if (from_user_pipe != -1) {
            
            pair<int,int> temp_pair = {from_user_pipe, user_idx};
            if (User_Info_Map.find(from_user_pipe) == User_Info_Map.end() || client_fd_table[from_user_pipe] == -1) {
                cout << "*** Error: user #" << from_user_pipe << " does not exist yet. ***" << endl;
                from_user_pipe = -1;
            } else if (UserPipes.find(temp_pair) != UserPipes.end()) {
                cout << "*** Error: the pipe #" << temp_pair.first << "->" << temp_pair.second << " already exists. ***" << endl;
                from_user_pipe = -1;
            } else {
                // nornal cases:
                string pipe_in_msg = "*** " + User_Info_Map.at(user_idx).UserName + "(#" + to_string(user_idx)
                                     + ") just received from " + User_Info_Map.at(to_user_pipe).UserName + "(#" 
                                     + to_string(to_user_pipe) + ") by '";
                for (size_t i = 0; i < info.argv[argvIndex].size(); ++i) {
                    pipe_in_msg += ((i == 0 ? "":" ") + info.argv[argvIndex][i]);
                }
                pipe_in_msg += "' ***";
                broadcast(pipe_in_msg, client_fd_table, User_Info_Map);
                dup2Client(client_fd_table[user_idx]);
            }
            info.argv[argvIndex].erase(info.argv[argvIndex].begin() + from_token_idx);
        }

        if (to_user_pipe != -1) {
            
            pair<int,int> temp_pair = {user_idx, to_user_pipe};
            if (User_Info_Map.find(to_user_pipe) == User_Info_Map.end() || client_fd_table[to_user_pipe] == -1) {
                cout << "*** Error: user #" << to_user_pipe << " does not exist yet. ***" << endl;
                to_user_pipe = -1;
            } else if (UserPipes.find(temp_pair) == UserPipes.end()) {
                cout << "*** Error: the pipe #" << temp_pair.first << "->" << temp_pair.second << "does not exist yet. ***" << endl;
                to_user_pipe = -1;
            } else {
                // normal cases
                bool dest_pipe_exist = false;
                for (auto &pipe: UserPipes) {
                    if (pipe.first.second == to_user_pipe) {
                        dest_pipe_exist = true;
                        break;
                    }
                }

                if (!dest_pipe_exist) {
                    UserPipes[temp_pair] = {-1, {}, {}, -1};
                    if (pipe(pipeMap[info.opOrder[argvIndex]].fd) < 0) {
                        cerr << "Error: Unable to create pipe" << endl;
                        exit(1);
                    }
                }

                string pipe_out_msg = "*** " + User_Info_Map.at(user_idx).UserName + "(#" + to_string(user_idx) + ") just piped '";
                for (size_t i = 0; i < info.argv[argvIndex].size(); ++i) {
                    pipe_out_msg += ((i == 0 ? "":" ") + info.argv[argvIndex][i]);
                }
                pipe_out_msg += ("' to " + User_Info_Map.at(to_user_pipe).UserName + "(#" + to_string(to_user_pipe) + ")");
                broadcast(pipe_out_msg, client_fd_table, User_Info_Map);
                dup2Client(client_fd_table[user_idx]);
            }
            info.argv[argvIndex].erase(info.argv[argvIndex].begin() + to_token_idx);
        }

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
            } else {
                for (auto &user: UserPipes) {
                    if (user.first.second == user_idx) {
                        close(user.second.fd[1]);
                        dup2(user.second.fd[0], STDIN_FILENO);
                        close(user.second.fd[0]);
                        break;
                    } 
                }
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
            } else {
                for (auto &user: UserPipes) {
                    if (user.first.first == user_idx) {
                        close(user.second.fd[0]);
                        dup2(user.second.fd[1], STDOUT_FILENO);
                        close(user.second.fd[1]);
                        break;
                    }
                }
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

            pair<int,int> temp_pair = {-1, -1};
            for (auto &user: UserPipes) {
                if (user.first.second == user_idx) {
                    close(user.second.fd[0]);
                    close(user.second.fd[1]);
                    temp_pair = user.first;
                    break;
                }
            }
            if (temp_pair.first != -1 && temp_pair.second != -1) {
                UserPipes.erase(temp_pair);
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
