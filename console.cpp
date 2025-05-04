#include <cstdlib>
#include <iostream>
#include <fstream>                       // ifstream
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>    // boost::replace
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <map>

using boost::asio::ip::tcp;
using namespace std;

struct UserInfo{
    string host;
    int port;
    string fileName;
};

map<string, string> environment_list;
map<size_t, UserInfo> UserInfoMap;

class Client : public enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& io_context, size_t i) : socket_(io_context), resolver_(io_context), index_(i) {}

    void start() {
        do_resolve();
    }

private:
    void do_resolve() {
        auto self(shared_from_this());
        resolver_.async_resolve(
            UserInfoMap[index_].host, to_string(UserInfoMap[index_].port), [this, self](boost::system::error_code ec, tcp::resolver::results_type result){
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
        boost::replace_all(msg, "\n", "&NewLine;");
        boost::replace_all(msg, "&", "&amp;");
        boost::replace_all(msg, "\r", "");
        boost::replace_all(msg, "\'", "&apos;");
        boost::replace_all(msg, "\"", "&quot;");
        boost::replace_all(msg, "<", "&lt;");
        boost::replace_all(msg, ">", "&gt;");
        cout << "<script>document.getElementById('s" << index_ << "').innerHTML += '"
             << (type == "command" ? "<b>":"") << msg << (type == "command" ? "<b>":"") << "';</script>\n" << flush;
    
    }

    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::results_type endpoint_;
    size_t index_;
    ifstream in;
    
    enum { max_length = 1024 };
    char data_[max_length];
};


int main(int argc, char* argv[]) {

    try {
        cout << "Context-type: text/html\r\n\r\n" << flush;

        // get environment set
        environment_list["REQUEST_METHOD"] = string(getenv("REQUEST_METHOD"));
        environment_list["REQUEST_URI"] = string(getenv("REQUEST_URI"));
        environment_list["QUERY_STRING"] = string(getenv("QUERY_STRING"));
        environment_list["SERVER_PROTOCOL"] = string(getenv("SERVER_PROTOCOL"));
        environment_list["HTTP_HOST"] = string(getenv("HTTP_HOST"));
        environment_list["SERVER_ADDR"] = string(getenv("SERVER_ADDR"));
        environment_list["SERVER_PORT"] = string(getenv("SERVER_PORT"));
        environment_list["REMOTE_ADDR"] = string(getenv("REMOTE_ADDR"));
        environment_list["REMOTE_PORT"] = string(getenv("REMOTE_PORT"));

        // query string parsing
        vector<string> each_query_string;
        size_t prev_start = 0;
        for (size_t i = 0; i < environment_list["QUERY_STRING"].size(); ++i) {
            if (environment_list["QUERY_STRING"][i] == 'h') {
                each_query_string.push_back(environment_list["QUERY_STRING"].substr(prev_start, i - prev_start));
                prev_start = i;
            }
        }

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
            string port_str = each_query_string[i].substr(p_start, f_start - 3 - p_start);
            if (port_str.empty()) {
                port_num = -1;
            } else {
                port_num = stoi(port_str);
            }
            UserInfoMap[i] = {
                each_query_string[i].substr(h_start, p_start - 3 - h_start), 
                port_num,
                each_query_string[i].substr(p_start)
            };

        }
        
        // from sample_console.cgi
        cout << "<!DOCTYPE html>"                                                                               << endl;
        cout << "<html lang=\"en\">"                                                                            << endl;
        cout << "<head>"                                                                                        << endl;
        cout << "    <meta charset=\"UTF-8\" />"                                                                << endl;
        cout << "    <title>NP Project 3 Sample Console</title>"                                                << endl;
        cout << "    <link"                                                                                     << endl;
        cout << "    rel=\"stylesheet\""                                                                        << endl;
        cout << "    href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\""          << endl;
        cout << "    integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\""     << endl;
        cout << "    crossorigin=\"anonymous\""                                                                 << endl;
        cout << "    />"                                                                                        << endl;
        cout << "    <link"                                                                                     << endl;
        cout << "    href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\""                          << endl;
        cout << "    rel=\"stylesheet\""                                                                        << endl;
        cout << "    />"                                                                                        << endl;
        cout << "    <link"                                                                                     << endl;
        cout << "    rel=\"icon\""                                                                              << endl;
        cout << "    type=\"image/png\""                                                                        << endl;
        cout << "    href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\""   << endl;
        cout << "    />"                                                                                        << endl;
        cout << "    <style>"                                                                                   << endl;
        cout << "    * {"                                                                                       << endl;
        cout << "        font-family: 'Source Code Pro', monospace;"                                            << endl;
        cout << "        font-size: 1rem !important;"                                                           << endl;
        cout << "    }"                                                                                         << endl;
        cout << "    body {"                                                                                    << endl;
        cout << "        background-color: #212529;"                                                          << endl;
        cout << "    }"                                                                                         << endl;
        cout << "    pre {"                                                                                     << endl;
        cout << "        color: #cccccc;"                                                                     << endl;
        cout << "    }"                                                                                         << endl;
        cout << "    b {"                                                                                       << endl;
        cout << "        color: #01b468;"                                                                     << endl;
        cout << "    }"                                                                                         << endl;
        cout << "    </style>"                                                                                  << endl;
        cout << "</head>"                                                                                       << endl;
        cout << "<body>"                                                                                        << endl;
        cout << "    <table class=\"table table-dark table-bordered\">"                                         << endl;
        cout << "    <thead>"                                                                                   << endl;
        cout << "        <tr>"                                                                                  << endl;
        for (auto user:UserInfoMap) {
            cout << "<th scope=\"col\">" << user.second.host << ":" << user.second.port << "</th>"              << endl;
        }
                //<th scope="col">nplinux1.cs.nycu.edu.tw:1234</th>
                //<th scope="col">nplinux2.cs.nycu.edu.tw:5678</th>
        cout << "        </tr>"                                                                                 << endl;
        cout << "    </thead>"                                                                                  << endl;
        cout << "    <tbody>"                                                                                   << endl;
        cout << "        <tr>"                                                                                  << endl;
        for (auto user:UserInfoMap) {
            cout << "<td><pre id=\"s" << user.first << "\" class=\"mb-" << user.first << "\"></pre></td>"       << endl;
        }
                //<td><pre id="s0" class="mb-0"></pre></td>
                //<td><pre id="s1" class="mb-0"></pre></td>
        cout << "        </tr>"                                                                                 << endl;
        cout << "    </tbody>"                                                                                  << endl;
        cout << "    </table>"                                                                                  << endl;
        cout << "</body>"                                                                                       << endl;
        cout << "</html>"                                                                                       << endl;

        boost::asio::io_context io_context;

        for (auto user:UserInfoMap) {
            make_shared<Client>(io_context, user.first)->start();
        }

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}