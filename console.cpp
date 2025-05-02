#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <cstring>
#include <string>
#include <map>



using boost::asio::ip::tcp;
using namespace std;

int main(int argc, char* argv[]) {

    map<string,string> environment_list;

    try {
        cout << "Context-type: text/html\r\n\r\n" << flush;

        environment_list["REQUEST_METHOD"] = string(getenv("REQUEST_METHOD"));
        environment_list["REQUEST_URI"] = string(getenv("REQUEST_URI"));
        environment_list["QUERY_STRING"] = string(getenv("QUERY_STRING"));
        environment_list["SERVER_PROTOCOL"] = string(getenv("SERVER_PROTOCOL"));
        environment_list["HTTP_HOST"] = string(getenv("HTTP_HOST"));
        environment_list["SERVER_ADDR"] = string(getenv("SERVER_ADDR"));
        environment_list["SERVER_PORT"] = string(getenv("SERVER_PORT"));
        environment_list["REMOTE_ADDR"] = string(getenv("REMOTE_ADDR"));
        environment_list["REMOTE_PORT"] = string(getenv("REMOTE_PORT"));


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
        /*
        for (size_t i = 0; i < something.size(); ++i) {
            cout << "<th scope=\"col\">" << nplinux1.cs.nycu.edu.tw << ":" << 1234 << "</th>"                   << endl;
        }
        */
                //<th scope="col">nplinux1.cs.nycu.edu.tw:1234</th>
                //<th scope="col">nplinux2.cs.nycu.edu.tw:5678</th>
        cout << "        </tr>"                                                                                 << endl;
        cout << "    </thead>"                                                                                  << endl;
        cout << "    <tbody>"                                                                                   << endl;
        cout << "        <tr>"                                                                                  << endl;
        /*
        for (size_t i = 0; i < something.size(); ++i) {
            cout << "<td><pre id=\"s" << i << "\" class=\"mb-" << i << "\"></pre></td>"                         << endl;
        }
        */
                //<td><pre id="s0" class="mb-0"></pre></td>
                //<td><pre id="s1" class="mb-0"></pre></td>
        cout << "        </tr>"                                                                                 << endl;
        cout << "    </tbody>"                                                                                  << endl;
        cout << "    </table>"                                                                                  << endl;
        cout << "</body>"                                                                                       << endl;
        cout << "</html>"                                                                                       << endl;

        boost::asio::io_context io_context;

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}