#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <sys/wait.h>       //waitpid()
#include <unistd.h>         //fork(), exec()
#include <cstring>
#include <string>
#include <map>              // map<string,string> environment_list

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

class session : public enable_shared_from_this<session> {
public:
    session(tcp::socket socket) : socket_(move(socket)) {}

    void start() {
        get_socks_requset();
    }

private:
    void get_socks_requset() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    int VN = data_[0];
                    int CD = data_[1];
                    int DSTPORT = ((int)(data_[2])) * 256 + ((int)(data_[3]));
                    string DSTIP = to_string(static_cast<int>(data_[4])) + "." + to_string(static_cast<int>(data_[5])) + "." + to_string(static_cast<int>(data_[6])) + "." + to_string(static_cast<int>(data_[7]));
                    cout << VN << endl;
                    cout << CD << endl;
                    cout << DSTPORT << endl;
                    cout << DSTIP << endl;
                }
            }
        );
    }
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    data_[length] = '\0';
                    
                    while(waitpid(-1, NULL, WNOHANG) > 0);

                    // 1. fork
                    pid_t pid = fork();

                    if (pid == 0) {
                        // 2. setenv
                        string request = string(data_);
                        vector<vector<string>> request_list = {{}};
                        size_t prev_start = 0;
                        for (size_t i = 0; i < request.size(); ++i) {
                            if (request[i] == ' ' && request_list[request_list.size() - 1].size() == 0) {
                                request_list[request_list.size() - 1].push_back(request.substr(prev_start, i - prev_start));
                                prev_start = i + 1;
                            } else if (request[i] == '\n' && request_list[request_list.size() - 1].size() == 1) {
                                request_list[request_list.size() - 1].push_back(request.substr(prev_start, i - prev_start));
                                prev_start = i + 1;
                                request_list.push_back({});
                                prev_start = ++i;
                            }
                        }

                        request_list.erase(request_list.begin() + request_list.size() - 1);

                        // 2-1. fill environment list
                        int space = request_list[0][1].find(' '), question_mark = request_list[0][1].find('?');
                        environment_list["REQUEST_METHOD"] = request_list[0][0];
                        environment_list["REQUEST_URI"] = request_list[0][1].substr(0, space);
                        environment_list["QUERY_STRING"] = question_mark == -1 ? "" : request_list[0][1].substr(question_mark + 1, space - question_mark);
                        environment_list["SERVER_PROTOCOL"] = request_list[0][1].substr(space + 1);
                        environment_list["HTTP_HOST"] = request_list[1][1];
                        environment_list["SERVER_ADDR"] = socket_.local_endpoint().address().to_string();
                        environment_list["SERVER_PORT"] = to_string(socket_.local_endpoint().port());
                        environment_list["REMOTE_ADDR"] = socket_.remote_endpoint().address().to_string();
                        environment_list["REMOTE_PORT"] = to_string(socket_.remote_endpoint().port());
                        cgi_path = question_mark == -1 ? 
                                    (space == -1 ? request_list[0][1]:request_list[0][1].substr(0, space)) :
                                    request_list[0][1].substr(0, question_mark);

                        for (auto pair: environment_list) {
                            setenv(pair.first.c_str(), pair.second.c_str(), 1);
                        }

                        // 3. dup
                        
                        dup2(socket_.native_handle(), STDIN_FILENO);
                        dup2(socket_.native_handle(), STDOUT_FILENO);
                        dup2(socket_.native_handle(), STDERR_FILENO);
                        
                        // 4. exec
                        if (environment_list["REQUEST_URI"].find(".cgi") == string::npos) {
                            cout << "HTTP/1.1 403 Forbiden\r\n" << flush;
                        } else {
                            cout << "HTTP/1.1 200 OK\r\n" << flush;
                            cout << "Server: http_server\r\n" << flush;

                            cgi_path = "." + cgi_path;

                            vector<string> vector_path = {cgi_path};
                            vector<char*> args;
                            for (auto &arg : vector_path) {
                                args.push_back(&arg[0]);
                            }
                            args.push_back(nullptr);

                            if (execvp(args[0], args.data()) == -1) {
                                exit(1);
                            }
                        }

                        exit(0);
                    } else {
                        socket_.close();
                        while(waitpid(-1, NULL, WNOHANG) > 0);
                    }
                }
            }
        );
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    map<string,string> environment_list;
    string cgi_path;
};

class server {
public:
    server(boost::asio::io_context& io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    io_context.notify_fork(boost::asio::io_context::fork_prepare);
                    pid_t pid = fork();

                    if (pid == 0) {
                        io_context.notify_fork(boost::asio::io_context::fork_child);
                        make_shared<session>(move(socket))->start();
                    } else {
                        io_context.notify_fork(boost::asio::io_context::fork_parent);
                        while (waitpid(-1, NULL, WNOHANG) > 0);
                        socket.close();
                    }
                }
                do_accept();
            }
        );
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            cerr << "Usage: socks_server <port>\n";
            return 1;
        }

        

        server s(io_context, atoi(argv[1]));

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
