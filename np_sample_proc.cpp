#include "np_sample_proc.hpp"

int client_fd_table[MAX_CLIENT];
map<int, UserInfo> User_Info_Map;


int main(int argc, char* argv[]) {
    
    int port = 7001;
    if (argv[1]) {
        port = stoi(string(argv[1]));
    }

    singleProcessConcurrentServer(port);

    return 0;
}

void singleProcessConcurrentServer(int port) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    struct sigaction sa;

    // create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "server socket error" << endl;
        exit(1);
    }

    // set socket opt: address user test
    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "set sockopt address error" << endl;
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1) {
        cerr << "set sockopt port error" << endl;
        exit(1);
    }

    // bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    memset(&(server_addr.sin_zero), 0, 8);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        cerr << "bind error" << endl;
        exit(1);
    }

    // listen
    if (listen(server_fd, MAX_CLIENT) == -1) {
        cerr << "listen error" << endl;
        exit(1);
    }

    // zombie processes ??
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        cerr << "sigaction error" << endl;
        exit(1);
    }
    signal(SIGCHLD, sigchld_handler);

    // file descripter handle

    fd_set all_fd_set, rset;
    
    for (size_t i = 0; i < MAX_CLIENT; ++i) {
        client_fd_table[i] = FD_UNDEFINED;
    }

    FD_ZERO(&all_fd_set);
    FD_SET(server_fd, &all_fd_set);

    // backup STD ???
    dup2(STDIN_FILENO, 4);
    dup2(STDOUT_FILENO, 5);
    dup2(STDERR_FILENO, 6);

    cout << "Server is listening on port:" << port << "..." << endl;

    int num_current_fd = FD_UNDEFINED, connect_fd;
    User_Info_Map.clear();

    int MAX_FD_NUM = 50, current_max_fd = -1;

    while (true) {
        rset = all_fd_set;
        if ((num_current_fd = select(MAX_FD_NUM+1, &rset, NULL, NULL, (struct timeval *)0)) == FD_UNDEFINED) {
            if (errno == EINTR)
                continue;
            cerr << "select error" << endl;
            continue;
        }

        if (FD_ISSET(server_fd, &rset)) {
            size_t i;
            sin_size = sizeof(client_addr);
            if ((connect_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size)) == FD_UNDEFINED) {
                cout << "accept error" << endl;
                continue;
            }

            for (i = 1; i < MAX_CLIENT; ++i) {
                if (client_fd_table[i] < 0) {
                    client_fd_table[i] = connect_fd;
                    break;
                }
            }

            if (i == MAX_CLIENT) {
                cerr << "Too many client" << endl;
                exit(1);
            }

            // User_Info_Map init
            inet_ntop(AF_INET, &(client_addr.sin_addr), User_Info_Map[i].IPAddress,INET_ADDRSTRLEN);
            User_Info_Map[i].port = ntohs(client_addr.sin_port);
            User_Info_Map[i].UserName = "(no name)";
            User_Info_Map[i].EnvVar["PATH"] = "bin:.";
            User_Info_Map[i].totalCommandCount = 0;
            User_Info_Map[i].pipeMap;
            // pipe handler ????

            /* Env var init */
            clearenv();
            for(auto env : User_Info_Map[i].EnvVar){
                setenv(env.first.c_str(), env.second.c_str(), 1);
            }

            dup2Client(client_fd_table[i]);
            welcomeMsg();
            string welmsg = "*** User '" + User_Info_Map[i].UserName + "' entered from " + User_Info_Map[i].IPAddress + ". ***";
            broadcast(welmsg);
            dup2Client(client_fd_table[i]);
            
            /* diff between connect_fd and client id*/
            FD_SET(connect_fd, &all_fd_set);
			if(connect_fd > MAX_FD_NUM) {
                MAX_FD_NUM = connect_fd;
            }
			if((int)i > current_max_fd) {
                current_max_fd = (int)i;
            } 
			if(--num_current_fd <= 0) {
                continue;
            }
        }
        for (size_t i = 1; (int)i <= current_max_fd; ++i) {
            if (client_fd_table[i] < 0) {
                continue;
            }

            if (FD_ISSET(client_fd_table[i], &all_fd_set)) {
                clearenv();
                for(auto env : User_Info_Map[i].EnvVar){
                    setenv(env.first.c_str(), env.second.c_str(), 1);
                }
                // counter = usersData[i].counter;
                // numberedPipes = usersData[i].numberedPipes;

                dup2Client(client_fd_table[i]);
                bool exit = false;
                User_Info_Map[i].pipeMap = npshell_handle_one_line(User_Info_Map[i].pipeMap, &exit, &User_Info_Map[i].totalCommandCount);
                if (exit) {
                    dup2(4, STDIN_FILENO);
                    dup2(5, STDOUT_FILENO);
                    dup2(6, STDERR_FILENO);
                    close(client_fd_table[i]);
                    FD_CLR(client_fd_table[i], &all_fd_set);
                    client_fd_table[i] = -1;
                    string logout_msg = "*** User '" + User_Info_Map[i].UserName + "' left. ***";
                    broadcast(logout_msg);
                    User_Info_Map.erase(i);
                }
                dup2(4, STDIN_FILENO);
                dup2(5, STDOUT_FILENO);
                dup2(6, STDERR_FILENO);
                if(--num_current_fd <= 0) {
                    break;
                }
            }
        }
    }
}

void dup2Client(int fd) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}

void welcomeMsg() {
    cout << "***************************************" << endl;
    cout << "** Welcom to the information server. **" << endl;
    cout << "***************************************" << endl;
}

/* Not broadcast  thinking*/
void broadcast(string msg) {
    for (auto user : User_Info_Map) {
        dup2Client(client_fd_table[user.first]);
        cout << msg << endl;
    }
}