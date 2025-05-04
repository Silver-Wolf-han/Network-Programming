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

class session : public enable_shared_from_this<session> {
public:
    session(tcp::socket socket) : socket_(move(socket)){}

    void start(){
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    // cout << "length:" << length << endl;
                    data_[length] = '\0';
                    // cout << endl << data_ << endl << flush;
                    // do_write(length);
                    
                    
                    while(waitpid(-1, NULL, WNOHANG) > 0);

                    // 1. fork
                    pid_t pid = fork();

                    if (pid == 0) {
                        // 2. setenv
                        string request = string(data_);
                        memset(data_, '\0', sizeof(data_));

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
                        /*
                        for (size_t i = 0; i < request_list.size(); ++i) {
                            for (size_t j = 0; j < request_list[i].size(); ++j) {
                                cout << "i:" << i << " j:" << j << endl;
                                cout << request_list[i][j] << endl;
                            }
                        }
                        */
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
                            // cout << pair.first << " " << pair.second << endl;
                        }
                        // cout << "path " << cgi_path << endl;

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
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    make_shared<session>(move(socket))->start();
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
            cerr << "Usage: http_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        server s(io_context, atoi(argv[1]));

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
