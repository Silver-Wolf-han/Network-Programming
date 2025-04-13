#include "np_single_proc.hpp"

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

    // server addr test
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    memset(&(server_addr.sin_zero), 0, 8);

    // bind
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        cerr << "bind error" << endl;
        exit(1);
    }

    // listen
    if (listen(server_fd, MAX_CLIENT) == -1) {
        cerr << "listen error" << endl;
        exit(1);
    }

    // zombie processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        cerr << "sigaction error" << endl;
        exit(1);
    }
    signal(SIGCHLD, sigchld_handler);

    // FD handle
    /* 1. Init client_fd_table */
    for (size_t i = 0; i < MAX_CLIENT; ++i) {
        client_fd_table[i] = FD_UNDEFINED;
    }

    /* 2. Init all_fd_set: managment all fd */
    fd_set all_fd_set;
    FD_ZERO(&all_fd_set);
    FD_SET(server_fd, &all_fd_set);
    
    // return value of select(), meaning that what number have reply from select
    int select_reply_fd_count = 0;
    // record max size in fd table
    size_t max_fd_in_table = 0;
    // fd variable
    int max_fd_history = 50, connect_fd = FD_UNDEFINED;

    /* 3. temp variable, recorder the status of fd_set before select */
    fd_set rset;

    User_Info_Map.clear();
    
    dup2(STDIN_FILENO, 4);
    dup2(STDOUT_FILENO, 5);
    dup2(STDERR_FILENO, 6);

    cout << "Server is listening on port:" << port << "..." << endl;

    while (true) {
        rset = all_fd_set;

        // select: management fd
        if ((select_reply_fd_count = select(max_fd_history+1, &rset, NULL, NULL, (struct timeval *)0)) == FD_UNDEFINED) {
            if (errno == EINTR)
                continue;
            cerr << "select error" << endl;
            continue;
        }
        if (FD_ISSET(server_fd, &rset)) {
            // accept
            sin_size = sizeof(client_addr);
            if ((connect_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size)) == FD_UNDEFINED) {
                cout << "accept error" << endl;
                continue;
            }

            // storge fd from accept (in local)
            size_t push_idx = 0;
            for (size_t i = 1; i < MAX_CLIENT; ++i) {
                if (client_fd_table[i] < 0) {
                    client_fd_table[i] = connect_fd;
                    push_idx = i;
                    break;
                }
            }
            if (push_idx == 0) {
                cerr << "size of client too much" << endl;
                exit(1);
            }

            // Init User_Info_Map
            inet_ntop(AF_INET, &(client_addr.sin_addr), User_Info_Map[(int)push_idx].IPAddress,INET_ADDRSTRLEN);
            User_Info_Map[(int)push_idx].port = ntohs(client_addr.sin_port);
            User_Info_Map[(int)push_idx].UserName = "(no name)";
            User_Info_Map[(int)push_idx].EnvVar["PATH"] = "bin:.";
            User_Info_Map[(int)push_idx].totalCommandCount = 0;
            User_Info_Map[(int)push_idx].pipeMap;

            // Enveriment Variable Init
            clearenv();
            for(auto env : User_Info_Map[(int)push_idx].EnvVar){
                setenv(env.first.c_str(), env.second.c_str(), 1);
            }

            // Welcome Mesage
            
            dup2Client(client_fd_table[(int)push_idx]);
            cout << "****************************************" << endl;
            cout << "** Welcome to the information server. **" << endl;
            cout << "****************************************" << endl;
            string welmsg = "*** User '" + User_Info_Map[(int)push_idx].UserName + 
                "' entered from " + User_Info_Map[(int)push_idx].IPAddress + 
                ":" + to_string(User_Info_Map[(int)push_idx].port) + ". ***";
            broadcast(welmsg, client_fd_table, User_Info_Map);
            dup2Client(client_fd_table[(int)push_idx]);
            typePrompt(false);

            // storge fd from accept (in FD_SET)
            FD_SET(connect_fd, &all_fd_set);

            // update fd num in history
			if(connect_fd > max_fd_history) {
                max_fd_history = connect_fd;
            }

            // update current fd max
			if(push_idx > max_fd_in_table) {
                max_fd_in_table = push_idx;
            }

            // if no one from accept then
            if (select_reply_fd_count <= 0) {
                continue;
            }
            --select_reply_fd_count;
        }

        for (size_t i = 1; i <= max_fd_in_table; ++i) {
            if (client_fd_table[i] < 0) {
                continue;
            }

            if (FD_ISSET(client_fd_table[i], &rset)) {
                clearenv();
                for(auto env : User_Info_Map[i].EnvVar){
                    setenv(env.first.c_str(), env.second.c_str(), 1);
                }

                dup2Client(client_fd_table[i]);

                bool exit = false;
                npshell_handle_one_line(User_Info_Map, i, &exit, client_fd_table);

                if (exit) {
                    dup2(4, STDIN_FILENO);
                    dup2(5, STDOUT_FILENO);
                    dup2(6, STDERR_FILENO);
                    close(client_fd_table[i]);
                    FD_CLR(client_fd_table[i], &all_fd_set);
                    client_fd_table[i] = -1;
                    string logout_msg = "*** User '" + User_Info_Map[i].UserName + "' left. ***";
                    broadcast(logout_msg, client_fd_table, User_Info_Map);
                    User_Info_Map.erase(i);
                }
                dup2(4, STDIN_FILENO);
                dup2(5, STDOUT_FILENO);
                dup2(6, STDERR_FILENO);
                if (select_reply_fd_count <= 0) {
                    break;
                }
                --select_reply_fd_count;
            }
        }
    }
}
