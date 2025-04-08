#include "np_sample.hpp"

using namespace std;


int main(int argc, char* argv[]) {
    
    int port = 7001;
    if (argv[1]) {
        port = stoi(string(argv[1]));
    }

    concurentConnectionOrientedServer(port);

    return 0;
}


void concurentConnectionOrientedServer(int port) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    struct sigaction sa;

    // create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "server socket error" << endl;
        exit(1);
    }

    // set socket opt
    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "set sockopt error" << endl;
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
    if (listen(server_fd, BACKLOG) == -1) {
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

    cout << "Server is listening on port:" << port << "..." << endl;

    // accept loop
    while (true) {

        sin_size = sizeof(struct sockaddr_in);
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            cout << "accept error" << endl;
            continue;
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Error: Unable to fork" << endl;
            exit(1);
        } else if (pid == 0) {
            close(server_fd);

            dup2(client_fd, STDIN_FILENO);
            dup2(client_fd, STDOUT_FILENO);
            dup2(client_fd, STDERR_FILENO);

            // run npshell here
            npshellInit();
            npshellLoop();

            close(client_fd);

            exit(0);
            
        } else {
            close(client_fd);
        }
        
    }

}
