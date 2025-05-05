#include <cstdlib>
#include <iostream>
#include <fstream>                       // ifstream
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>    // boost::replace
#include <winbase.h>
#include <cstring>
#include <string>
#include <vector>
#include <map>              // map<string,string> environment_list

using boost::asio::ip::tcp;
using namespace std;

string panel() {
    const int N_SERVERS = 5;
    const string FORM_METHOD = "GET";
    const string FORM_ACTION = "console.cgi";
    const string DOMAIN = "cs.nycu.edu.tw";

    string test_case_menu;
    for (int i = 1; i <= 5; ++i) {
        test_case_menu += "<option value=\"" + to_string(i) + ".txt\">" + to_string(i) + ".txt</option>";
    }

    string host_menu;
    for (int i = 1; i <= 12; ++i) {
        string host = "nplinux" + to_string(i);
        host_menu += "<option value=\"" + host + "." + DOMAIN + "\">" + host + "</option>";
    }
    string panel_context = "";
    panel_context += "Content-type: text/html\r\n\r\n";

    panel_context += R"(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>NP Project 3 Panel</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
      integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2" crossorigin="anonymous" />
    <link href="https://fonts.googleapis.com/css?family=Source+Code+Pro" rel="stylesheet" />
    <link rel="icon" type="image/png" href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png" />
    <style>* { font-family: 'Source Code Pro', monospace; }</style>
  </head>
  <body class="bg-secondary pt-5">
    <form action=")" + FORM_ACTION + R"(" method=")" + FORM_METHOD + R"(">
      <table class="table mx-auto bg-light" style="width: inherit">
        <thead class="thead-dark">
          <tr>
            <th scope="col">#</th>
            <th scope="col">Host</th>
            <th scope="col">Port</th>
            <th scope="col">Input File</th>
          </tr>
        </thead>
        <tbody>)";

    for (int i = 0; i < N_SERVERS; ++i) {
        panel_context += R"(
          <tr>
            <th scope="row" class="align-middle">Session )" + to_string(i + 1) + R"(</th>
            <td>
              <div class="input-group">
                <select name="h)" + to_string(i) + R"(" class="custom-select">
                  <option></option>)" + host_menu + R"(
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nycu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p)" + to_string(i) + R"(" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f)" + to_string(i) + R"(" class="custom-select">
                <option></option>)" + test_case_menu + R"(
              </select>
            </td>
          </tr>)";
    }

    panel_context += R"(
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
</html>)";
    return panel_context;
}

struct UserInfo{
    string host;
    int port;
    string fileName;
};

map<string, string> environment_list;
map<size_t, UserInfo> UserInfoMap;

void send(tcp::socket &socket, const string &msg) {
	boost::asio::write(socket, boost::asio::buffer(msg));
}

class Client : public enable_shared_from_this<Client> {
public:
    Client(tcp::socket &socket, boost::asio::io_context& io_context, size_t i) : out(socket), socket_(io_context), resolver_(io_context), index_(i) {}

    void start() {
        do_resolve();
    }

private:
    void do_resolve() {
        auto self(shared_from_this());
        resolver_.async_resolve(
            UserInfoMap[index_].host, to_string(UserInfoMap[index_].port), [this, self](boost::system::error_code ec, tcp::resolver::results_type result) {
                if(!ec) {
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
            socket_, endpoint_, [this, self](boost::system::error_code ec, tcp::endpoint ed){
                if(!ec) {
                    memset(data_, '\0', sizeof(data_));
                    in.open("./test_case/" + UserInfoMap[index_].fileName);
                    if(!in.is_open()) {
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
            boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, size_t length) {
                if (!ec && length) {
                    data_[length] = '\0';
                    string output_msg = string(data_);
                    output(output_msg, "result");
                    if (output_msg.find("% ") == string::npos) {
                        do_read();
                    } else {
                        do_write();
                    }
                } else if(!length) {
                    socket_.close();
                }
            }
        );
    }

    void do_write() {
        auto self(shared_from_this());
        string input_cmd;
        getline(in, input_cmd);
        input_cmd += '\n';
        output(input_cmd, "command");
        boost::asio::async_write(
            socket_, boost::asio::buffer(input_cmd, input_cmd.size()), [this, self](boost::system::error_code ec, size_t) {
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
        send(out, console_context);
    
    }

    tcp::socket &out;
    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::results_type endpoint_;
    size_t index_;
    ifstream in;
    
    enum { max_length = 1024 };
    char data_[max_length];
};


void console_cgi(tcp::socket &socket_){

    try {
        // query string parsing
        vector<string> each_query_string;
        size_t prev_start = 0;
        for (size_t i = 0; i < environment_list["QUERY_STRING"].size(); ++i) {
            if (environment_list["QUERY_STRING"][i] == 'h' && i) {
                each_query_string.push_back(environment_list["QUERY_STRING"].substr(prev_start, i - prev_start));
                prev_start = i;
            }
        }
        each_query_string.push_back(environment_list["QUERY_STRING"].substr(prev_start, environment_list["QUERY_STRING"].size() - prev_start));

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
        
        // from sample_console.cgi
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

        send(socket_, console_context);

        boost::asio::io_context io_context;

        for (auto user:UserInfoMap) {
            make_shared<Client>(socket_, io_context, user.first)->start();
        }

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
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
            boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    // cout << "length:" << length << endl;
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
                        SetEnvironmentVariable(pair.first.c_str(), pair.second.c_str());
                    }

                    // string request_reply;
                    if (environment_list["REQUEST_URI"].find(".cgi") == string::npos) {
                        // request_reply = "HTTP/1.1 403 Forbiden\r\n";
                        do_write("HTTP/1.1 403 Forbiden\r\n");
                        // cout << "HTTP/1.1 403 Forbiden\r\n" << flush;
                    } else {
                        // request_reply = "HTTP/1.1 200 OK\r\nServer: http_server\r\n";
                        do_write("HTTP/1.1 200 OK\r\nServer: http_server\r\n");
                        // cout << "HTTP/1.1 200 OK\r\n" << flush;
                        // cout << "Server: http_server\r\n" << flush;

                        if (cgi_path == "/panel.cgi") {
                            do_write(panel());
                            socket_.close();
                        } else if (cgi_path == "/console.cgi") {
                            console_cgi(socket_);
							// thread console_cgi_handler(console_cgi,  ref(socket_));
							// console_cgi_handler.detach();
                        }
                        do_read();
                    }
                }
            }
        );
    }

    void do_write(string out) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(out, out.size()), [this, self](boost::system::error_code ec, size_t) {}
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