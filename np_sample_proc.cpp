#include "np_sample_proc.hpp"

using namespace std;

map<int, User> UserMap;
map<int, map<int, int[2]>> UserPipesMap;



int main() {
    int listener, newfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    char buffer[4096];

    // create socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "server socket error" << endl;
        exit(1);
    }

    // set socket opt
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "set sockopt error" << endl;
        exit(1);
    }

    // bind
    int port = 7001;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    memset(&(server_addr.sin_zero), 0, 8);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        cerr << "bind error" << endl;
        exit(1);
    }

    // listen
    if (listen(listener, MAX_CLIENT) == -1) {
        cerr << "listen error" << endl;
        exit(1);
    }

    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(listener, &master_set);
    int fdmax = listener;

    int user_id_counter = 1;

    while(true) {
        read_fds = master_set;

        if (select(fdmax+1, &read_fds, nullptr, nullptr, nullptr) == -1) {
            cerr << "select error" << endl;
            exit(4);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    // New connection
                    addrlen = sizeof(client_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
                        cout << "accept error" << endl;
                        continue;
                    }

                    if (newfd != -1) {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax) {
                            fdmax = newfd;
                        }

                        //Add to user Map
                        User user({
                            user_id_counter++, 
                            newfd,
                            "(no name)",
                            inet_ntoa(client_addr.sin_addr),
                            ntohs(client_addr.sin_port)
                        });
                        
                        UserMap[newfd] = user;
                        string welcome = "User enter!";
                        broadcast(welcome);
                    }
                } else {
                    memset(buffer, 0, sizeof(buffer));
                    /*** problem ?? should not use recv */
                    int nbytes = recv(i, buffer, sizeof(buffer), 0);
                    if (nbytes <= 0) {
                        // User disconnected
                        string logout_msg = "User Logout";
                        broadcast(logout_msg);
                        close(i);
                        FD_CLR(i, &master_set);
                        UserMap.erase(i);
                    } else {
                        string input(buffer);

                        // My own example

                        if (input.find("yell") == 0) {
                            string yell_msg = "yell_msg";
                            broadcast(yell_msg);
                        }

                        if (input.find("|>") != string::npos) {
                            int to_id = stoi(input.substr(input.find("|>") + 2));
                            for (const auto& [sock, user] : UserMap) {
                                cout << sock << " " << user.id << endl;
                                int pipe_fd[2];
                                pipe(pipe_fd);
                                UserPipesMap[UserMap[i].id][to_id][0] = pipe_fd[0];
                                UserPipesMap[UserMap[i].id][to_id][1] = pipe_fd[1];

                                string pipe_msg = "pipe_msg";
                                broadcast(pipe_msg);
                            }
                        }


                        dup2(i, STDIN_FILENO);
                        dup2(i, STDOUT_FILENO);
                        dup2(i, STDERR_FILENO);

                        npshellInit();
                        npshellLoop();
                    }
                }
            }
        }

    }

    return 0;
}

void broadcast(const string& msg) {
    for (const auto& [sock, user] : UserPipesMap) {
        /*** problem?? should not use send */
        send(sock, msg.c_str(), msg.size(), 0);
    }
}

void send_to_user(int sock, const string& msg) {
    /*** problem?? should not use send */
    send(sock, msg.c_str(), msg.size(), 0);
}