#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <thread>

using boost::asio::ip::tcp;
using namespace std;

struct UserInfo {
    string host;
    int port;
    string fileName;
};

class Clinet : public enable_shared_from_this<Clinet> {
public:
Clinet(tcp::socket &socket, map<size_t, UserInfo> UserInfoMap, boost::asio::io_context& io_context, size_t index) : outSocket_(socket), UserInfoMap(UserInfoMap), resolver_(io_context), socket_(io_context), index_(index) {}

    void start() {
        do_resolve();
    }

private:
    void do_resolve() {
        auto self(shared_from_this());
        resolver_.async_resolve(
            UserInfoMap[index_].host, to_string(UserInfoMap[index_].port), [this, self](boost::system::error_code ec, tcp::resolver::results_type result) {
                if (!ec) {
                    memset(data_, '\0', sizeof(data_));
                    endpoint_ = result;
                    do_connect();
                } else {
                    socket_.close();
                }
            }
        );
    }
    
    void do_connect() {
        auto self(shared_from_this());
        boost::asio::async_connect(
            socket_, endpoint_, [this, self](boost::system::error_code ec, tcp::endpoint ed) {
                if (!ec) {
                    memset(data_, '\0', sizeof(data_));
                    in_.open("./test_case/" + UserInfoMap[index_].fileName);
                    if (!in_.is_open()) {
                        socket_.close();
                    }
                    do_read();
                } else {
                    socket_.close();
                }
            }
        );
    }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),  [this, self](boost::system::error_code ec, size_t length) {
                if (!ec && length) {
                    data_[length] = '\0';
                    string msg = string(data_);
                    memset(data_, '\0', sizeof(data_));
                    output(msg, "result");
                    if (msg.find("% ") == string::npos) {
                        do_read();
                    } else {
                        do_write();
                    }
                } else {
                    socket_.close();
                }
            }
        );
    }

    void do_write() {
        auto self(shared_from_this());
        string input_command;
        getline(in_, input_command);
        input_command.push_back('\n');
        output(input_command, "command");
        boost::asio::async_write(
            socket_, boost::asio::buffer(input_command, input_command.size()), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    do_read();
                }
            }
        );
    }
    
    void output(string msg, string type) {
        // html.escape
        boost::replace_all(msg, "&", "&amp;");
        boost::replace_all(msg, "\r", "");
        boost::replace_all(msg, "\n", "&NewLine;");
        boost::replace_all(msg, "\'", "&apos;");
        boost::replace_all(msg, "\"", "&quot;");
        boost::replace_all(msg, "<", "&lt;");
        boost::replace_all(msg, ">", "&gt;");
        
        string console_context = "<script>document.getElementById('s" + to_string(index_) + "').innerHTML += '"
             + (type == "command" ? "<b>":"") + msg + (type == "command" ? "<b>":"") + "';</script>\n";
        boost::asio::write(outSocket_, boost::asio::buffer(console_context));    
    }

    tcp::socket &outSocket_;
    map<size_t, UserInfo> UserInfoMap;
    tcp::resolver resolver_;
    tcp::socket socket_;
    size_t index_;
    tcp::resolver::results_type endpoint_;
    ifstream in_;
    enum { max_length = 40960 };
    char data_[max_length];
};

void console(tcp::socket &socket_, map<string, string> &env) {
    try {
        map<size_t, UserInfo> UserInfoMap;

        vector<string> each_query_string;
        size_t prev_start = 0;
        for (size_t i = 0; i < env["QUERY_STRING"].size(); ++i) {
            if (env["QUERY_STRING"][i] == 'h' && i) {
                each_query_string.push_back(env["QUERY_STRING"].substr(prev_start, i - prev_start));
                prev_start = i;
            }
        }
        each_query_string.push_back(env["QUERY_STRING"].substr(prev_start, env["QUERY_STRING"].size() - prev_start));

        for (size_t i = 0; i < each_query_string.size(); ++i) {
            size_t h_start = 0, p_start = 0, f_start = 0;
            for (size_t j = 0; j < each_query_string[i].size(); ++j) {
                switch(each_query_string[i][j]) {
                case 'h':
                    h_start = j + 3;
                    break;
                case 'p':
                    p_start = j + 3;
                    break;
                case 'f':
                    f_start = j + 3;
                    break;
                default:
                    break;
                }
            }

            int port_num;
            string port_str = each_query_string[i].substr(p_start, f_start - 3 - p_start - 1);
            if (port_str.empty()) {
                port_num = -1;
            } else {
                port_num = stoi(port_str);
            }
            UserInfoMap[i] = {
                each_query_string[i].substr(h_start, p_start - 3 - h_start - 1), 
                port_num,
                each_query_string[i].substr(f_start, each_query_string[i].size() - f_start - 1)
            };
        }

        string console_context = "";
        console_context += "<!DOCTYPE html>\n";
        console_context += "<html lang=\"en\">\n";
        console_context += "<head>\n";
        console_context += "    <meta charset=\"UTF-8\" />\n";
        console_context += "    <title>NP Project 3 Sample Console</title>\n";
        console_context += "    <link\n";
        console_context += "    rel=\"stylesheet\"\n";
        console_context += "    href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n";
        console_context += "    integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n";
        console_context += "    crossorigin=\"anonymous\"\n";
        console_context += "    />\n";
        console_context += "    <link\n";
        console_context += "    href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n";
        console_context += "    rel=\"stylesheet\"\n";
        console_context += "    />\n";
        console_context += "    <link\n";
        console_context += "    rel=\"icon\"\n";
        console_context += "    type=\"image/png\"\n";
        console_context += "    href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n";
        console_context += "    />\n";
        console_context += "    <style>\n";
        console_context += "    * {\n";
        console_context += "        font-family: 'Source Code Pro', monospace;\n";
        console_context += "        font-size: 1rem !important;\n";
        console_context += "    }\n";
        console_context += "    body {\n";
        console_context += "        background-color: #212529;\n";
        console_context += "    }\n";
        console_context += "    pre {\n";
        console_context += "        color: #cccccc;\n";
        console_context += "    }\n";
        console_context += "    b {\n";
        console_context += "        color: #01b468;\n";
        console_context += "    }\n";
        console_context += "    </style>\n";
        console_context += "</head>\n";
        console_context += "<body>\n";
        console_context += "    <table class=\"table table-dark table-bordered\">\n";
        console_context += "    <thead>\n";
        console_context += "        <tr>\n";
        for (auto user:UserInfoMap) {
            if (user.second.port != -1) {
                console_context += "<th scope=\"col\">" + user.second.host + ":" + to_string(user.second.port) + "</th>\n";
            }
        }
                //<th scope="col">nplinux1.cs.nycu.edu.tw:1234</th>
                //<th scope="col">nplinux2.cs.nycu.edu.tw:5678</th>
        console_context += "        </tr>\n";
        console_context += "    </thead>\n";
        console_context += "    <tbody>\n";
        console_context += "        <tr>\n";
        for (auto user:UserInfoMap) {
            if (user.second.port != -1) {
                console_context += "<td><pre id=\"s" + to_string(user.first) + "\" class=\"mb-" + to_string(user.first) + "\"></pre></td>\n";
            }
        }
                //<td><pre id="s0" class="mb-0"></pre></td>
                //<td><pre id="s1" class="mb-0"></pre></td>
        console_context += "        </tr>\n";
        console_context += "    </tbody>\n";
        console_context += "    </table>\n";
        console_context += "</body>\n";
        console_context += "</html>\n";
        
        boost::asio::write(socket_, boost::asio::buffer(console_context));

        boost::asio::io_context io_context;
        for (size_t i = 0; i < UserInfoMap.size(); i++) {
            make_shared<Clinet>(socket_ ,UserInfoMap, io_context, i)->start();
        }
        io_context.run();
        socket_.close();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
}

string panel_cgi() {
    string panel_context = R"(
<!DOCTYPE html>
<html lang="en">
    <head>
        <title>NP Project 3 Panel</title>
        <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css" integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2" crossorigin="anonymous"/>
        <link href="https://fonts.googleapis.com/css?family=Source+Code+Pro" rel="stylesheet"/>
        <link rel="icon" type="image/png" href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"/>
        <style> * {font-family: 'Source Code Pro', monospace;}</style>
    </head>
    <body class="bg-secondary pt-5">
        <form action="console.cgi" method="GET">
            <table class="table mx-auto bg-light" style="width: inherit">
                <thead class="thead-dark">
                    <tr>
                        <th scope="col">#</th><th scope="col">Host</th><th scope="col">Port</th><th scope="col">Input File</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <th scope="row" class="align-middle">Session 1</th>
                        <td>
                            <div class="input-group">
                                <select name="h0" class="custom-select">
                                    <option></option>
                                    <option value="nplinux1.cs.nycu.edu.tw">nplinux1</option>
                                    <option value="nplinux2.cs.nycu.edu.tw">nplinux2</option>
                                    <option value="nplinux3.cs.nycu.edu.tw">nplinux3</option>
                                    <option value="nplinux4.cs.nycu.edu.tw">nplinux4</option>
                                    <option value="nplinux5.cs.nycu.edu.tw">nplinux5</option>
                                    <option value="nplinux6.cs.nycu.edu.tw">nplinux6</option>
                                    <option value="nplinux7.cs.nycu.edu.tw">nplinux7</option>
                                    <option value="nplinux8.cs.nycu.edu.tw">nplinux8</option>
                                    <option value="nplinux9.cs.nycu.edu.tw">nplinux9</option>
                                    <option value="nplinux10.cs.nycu.edu.tw">nplinux10</option>
                                    <option value="nplinux11.cs.nycu.edu.tw">nplinux11</option>
                                    <option value="nplinux12.cs.nycu.edu.tw">nplinux12</option>
                                </select>
                                <div class="input-group-append">
                                    <span class="input-group-text">.cs.nycu.edu.tw</span>
                                </div>
                            </div>
                        </td>
                        <td>
                            <input name="p0" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                            <select name="f0" class="custom-select">
                                <option></option>
                                <option value="t1.txt">t1.txt</option>
                                <option value="t2.txt">t2.txt</option>
                                <option value="t3.txt">t3.txt</option>
                                <option value="t4.txt">t4.txt</option>
                                <option value="t5.txt">t5.txt</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 2</th>
                        <td>
                            <div class="input-group">
                                <select name="h1" class="custom-select">
                                    <option></option>
                                    <option value="nplinux1.cs.nycu.edu.tw">nplinux1</option>
                                    <option value="nplinux2.cs.nycu.edu.tw">nplinux2</option>
                                    <option value="nplinux3.cs.nycu.edu.tw">nplinux3</option>
                                    <option value="nplinux4.cs.nycu.edu.tw">nplinux4</option>
                                    <option value="nplinux5.cs.nycu.edu.tw">nplinux5</option>
                                    <option value="nplinux6.cs.nycu.edu.tw">nplinux6</option>
                                    <option value="nplinux7.cs.nycu.edu.tw">nplinux7</option>
                                    <option value="nplinux8.cs.nycu.edu.tw">nplinux8</option>
                                    <option value="nplinux9.cs.nycu.edu.tw">nplinux9</option>
                                    <option value="nplinux10.cs.nycu.edu.tw">nplinux10</option>
                                    <option value="nplinux11.cs.nycu.edu.tw">nplinux11</option>
                                    <option value="nplinux12.cs.nycu.edu.tw">nplinux12</option>
                                </select>
                                <div class="input-group-append">
                                    <span class="input-group-text">.cs.nycu.edu.tw</span>
                                </div>
                            </div>
                        </td>
                    <td>
                        <input name="p1" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                        <select name="f1" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option>
                            <option value="t2.txt">t2.txt</option>
                            <option value="t3.txt">t3.txt</option>
                            <option value="t4.txt">t4.txt</option>
                            <option value="t5.txt">t5.txt</option>
                        </select>
                    </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 3</th>
                        <td>
                            <div class="input-group">
                                <select name="h2" class="custom-select">
                                    <option></option>
                                    <option value="nplinux1.cs.nycu.edu.tw">nplinux1</option>
                                    <option value="nplinux2.cs.nycu.edu.tw">nplinux2</option>
                                    <option value="nplinux3.cs.nycu.edu.tw">nplinux3</option>
                                    <option value="nplinux4.cs.nycu.edu.tw">nplinux4</option>
                                    <option value="nplinux5.cs.nycu.edu.tw">nplinux5</option>
                                    <option value="nplinux6.cs.nycu.edu.tw">nplinux6</option>
                                    <option value="nplinux7.cs.nycu.edu.tw">nplinux7</option>
                                    <option value="nplinux8.cs.nycu.edu.tw">nplinux8</option>
                                    <option value="nplinux9.cs.nycu.edu.tw">nplinux9</option>
                                    <option value="nplinux10.cs.nycu.edu.tw">nplinux10</option>
                                    <option value="nplinux11.cs.nycu.edu.tw">nplinux11</option>
                                    <option value="nplinux12.cs.nycu.edu.tw">nplinux12</option>
                                </select>
                                <div class="input-group-append">
                                    <span class="input-group-text">.cs.nycu.edu.tw</span>
                                </div>
                            </div>
                        </td>
                        <td>
                            <input name="p2" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                            <select name="f2" class="custom-select">
                                <option></option>
                                <option value="t1.txt">t1.txt</option>
                                <option value="t2.txt">t2.txt</option>
                                <option value="t3.txt">t3.txt</option>
                                <option value="t4.txt">t4.txt</option>
                                <option value="t5.txt">t5.txt</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 4</th>
                        <td>
                            <div class="input-group">
                                <select name="h3" class="custom-select">
                                    <option></option>
                                    <option value="nplinux1.cs.nycu.edu.tw">nplinux1</option>
                                    <option value="nplinux2.cs.nycu.edu.tw">nplinux2</option>
                                    <option value="nplinux3.cs.nycu.edu.tw">nplinux3</option>
                                    <option value="nplinux4.cs.nycu.edu.tw">nplinux4</option>
                                    <option value="nplinux5.cs.nycu.edu.tw">nplinux5</option>
                                    <option value="nplinux6.cs.nycu.edu.tw">nplinux6</option>
                                    <option value="nplinux7.cs.nycu.edu.tw">nplinux7</option>
                                    <option value="nplinux8.cs.nycu.edu.tw">nplinux8</option>
                                    <option value="nplinux9.cs.nycu.edu.tw">nplinux9</option>
                                    <option value="nplinux10.cs.nycu.edu.tw">nplinux10</option>
                                    <option value="nplinux11.cs.nycu.edu.tw">nplinux11</option>
                                    <option value="nplinux12.cs.nycu.edu.tw">nplinux12</option>
                                </select>
                                <div class="input-group-append">
                                    <span class="input-group-text">.cs.nycu.edu.tw</span>
                                </div>
                            </div>
                        </td>
                        <td>
                            <input name="p3" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                            <select name="f3" class="custom-select">
                                <option></option>
                                <option value="t1.txt">t1.txt</option>
                                <option value="t2.txt">t2.txt</option>
                                <option value="t3.txt">t3.txt</option>
                                <option value="t4.txt">t4.txt</option>
                                <option value="t5.txt">t5.txt</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 5</th>
                        <td>
                            <div class="input-group">
                                <select name="h4" class="custom-select">
                                    <option></option>
                                    <option value="nplinux1.cs.nycu.edu.tw">nplinux1</option>
                                    <option value="nplinux2.cs.nycu.edu.tw">nplinux2</option>
                                    <option value="nplinux3.cs.nycu.edu.tw">nplinux3</option>
                                    <option value="nplinux4.cs.nycu.edu.tw">nplinux4</option>
                                    <option value="nplinux5.cs.nycu.edu.tw">nplinux5</option>
                                    <option value="nplinux6.cs.nycu.edu.tw">nplinux6</option>
                                    <option value="nplinux7.cs.nycu.edu.tw">nplinux7</option>
                                    <option value="nplinux8.cs.nycu.edu.tw">nplinux8</option>
                                    <option value="nplinux9.cs.nycu.edu.tw">nplinux9</option>
                                    <option value="nplinux10.cs.nycu.edu.tw">nplinux10</option>
                                    <option value="nplinux11.cs.nycu.edu.tw">nplinux11</option>
                                    <option value="nplinux12.cs.nycu.edu.tw">nplinux12</option>
                                </select>
                                <div class="input-group-append">
                                    <span class="input-group-text">.cs.nycu.edu.tw</span>
                                </div>
                            </div>
                        </td>
                        <td>
                            <input name="p4" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                            <select name="f4" class="custom-select">
                                <option></option>
                                <option value="t1.txt">t1.txt</option>
                                <option value="t2.txt">t2.txt</option>
                                <option value="t3.txt">t3.txt</option>
                                <option value="t4.txt">t4.txt</option>
                                <option value="t5.txt">t5.txt</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3"></td>
                        <td>
                            <button type="submit" class="btn btn-info btn-block">Run</button>
                        </td>
                    </tr>
                </tbody>
            </table>
        </form>
    </body>
</html>
)";
    return panel_context;
    
}

class session : public enable_shared_from_this<session> {
public:
    session(tcp::socket socket) : socket_(move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length), 
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    data_[length] = '\0';

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
                    
                    // 2-1. fill environment list
                    int space = request_list[0][1].find(' '), question_mark = request_list[0][1].find('?');
                    env["REQUEST_METHOD"] = request_list[0][0];
                    env["REQUEST_URI"] = request_list[0][1].substr(0, space);
                    env["QUERY_STRING"] = question_mark == -1 ? "" : request_list[0][1].substr(question_mark + 1, space - question_mark - 1);
                    env["SERVER_PROTOCOL"] = request_list[0][1].substr(space + 1);
                    env["HTTP_HOST"] = request_list[1][1];
                    env["SERVER_ADDR"] = socket_.local_endpoint().address().to_string();
                    env["SERVER_PORT"] = to_string(socket_.local_endpoint().port());
                    env["REMOTE_ADDR"] = socket_.remote_endpoint().address().to_string();
                    env["REMOTE_PORT"] = to_string(socket_.remote_endpoint().port());
                    path = question_mark == -1 ? 
                                (space == -1 ? request_list[0][1]:request_list[0][1].substr(0, space)) :
                                request_list[0][1].substr(0, question_mark);
                    
                    if (env["REQUEST_URI"].find(".cgi") == string::npos) {
                        do_write("HTTP/1.1 403 Forbiden\r\n");
                    } else if (path == "/panel.cgi") {
                        do_write("HTTP/1.1 200 OK\r\nServer: http_server\r\nContent-type: text/html\r\n\r\n");
                        do_write(panel_cgi());
                        socket_.close();
                    } else if (path == "/console.cgi") {
                        do_write("HTTP/1.1 200 OK\r\nServer: http_server\r\nContent-type: text/html\r\n\r\n");
                        thread console_cgi_handler(console,  ref(socket_), ref(env));
                        console_cgi_handler.detach();
                    } else {
                        do_write("HTTP/1.1 404 NOT FOUND\r\n\r\n");
                    }
                    do_read();
                }
            }
        );
    }
    
    void do_write(string out) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(out, out.size()), [this, self](boost::system::error_code ec, size_t length) {}
        );
    }
    
    tcp::socket socket_;
    map<string, string> env;
    string path;
    int state;
    enum { max_length = 4096 };
    char data_[max_length];
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
            cerr << "Usage: .\\cgi_server.exe <port>\n";
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
