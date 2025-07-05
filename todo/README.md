# Project 4

## Part 1 Linux Version

### http_server.cpp
`[Usage]: ./http_server [port]`
(According to Spec) What needs to be done here is
1. Handle the HTTP URL (e.g., `http://127.0.0.1:7001/panel.cgi`)
2. Call and execute the `something.cgi` in the URL
3. Set up environment variables according to the URL
4. Implement using `<boost/asio.hpp>`

#### main
(Copied directly from `extra_files/echo_server.cpp`'s `main`, no changes)
Just instantiates `class server`

#### class server
(Copied directly from `extra_files/echo_server.cpp`'s `server`, no changes)
Just responsible for accepting connections. The `constructor` only calls `do_accept()`
Then, after `do_accept()` receives a connection, it spawns a `class session` to handle it, and continues accepting recursively
[Boost async_accept](https://beta.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_socket_acceptor/async_accept/overload6.html)
```=cpp
void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error ec, tcp::socket socket) {
            if (!ec) {
                make_shared<session>(move(socket))->start();
            }
            do_accept();
        }
    );
}
tcp::acceptor acceptor_;
```
The function flow is:
1. Enter `void do_accept()`
2. Enter `acceptor_.async_accept()` with 1 argument
    2-1.  A lambda function  `[this](boost::system::error ec, tcp::socket socket)` (called "handler" in the docs)
3. **After `async_accept()` finishes, the lambda function is called**
4. The lambda creates a `class session` to handle the request

What is `make_shared<session>(move(socket))->start();`?
About `make_shared` [StackOverfolw](https://stackoverflow.com/questions/50312423/boostmake-shared-fail-but-stdmake-shared-work), [Boost](https://www.boost.org/doc/libs/1_44_0/libs/smart_ptr/make_shared.html) 反正我要很粗暴的理解為就是一個 shared pointer 啦

#### class session
(The structure is also copied from `extra_file/echo_server.cpp`, mainly adding logic inside `do_read()`, and there's no `do_write()`, because we directly execute `some.cgi`)
This part finishes the remaining spec requirements
```=cpp
class session: public enable_shared_from_this<session> {};
```
(Let's crudely interpret this as matching with `make_shared` above)
```=cpp
void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(
        boost::asio::buffer(date_, max_length), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                fork()
                child_1: fill environment
                    // politely called "parsing", bluntly "string processing"
                child_2: dup to client
                child_3: exec `cgi`
                    3-1. Output http protocol (ending in "\r\n")
                    3-2. Execute
                parent: close `socket_`, leave it to the child
            }
        }
    );
}
```
1. ~~String processing~~ Parsing according to spec
2. `dup(socket_.native_handle(), FD)`yep, see [Boost](https://live.boost.org/doc/libs/1_87_0/doc/html/boost_asio/reference/basic_stream_socket/native_handle.html)
3. `exec .cgi`
    3-1. Extract URL; if no `.cgi`, send `"HTTP/1.1 403 Forbiden\r\n"` back
    3-2. If `.cgi` present, send `"HTTP/1.1 200 OK\r\nServer: http_server\r\n"`
    3-3. Execute `.cgi` (`panel.cgi` is given by TA, `console.cgi` is created below; theoretically, after clicking run on `panel.cgi`, the URL changes to `console.cgi`)
    
Boost Function Note:
1. `socket_.async_read_some()` [Boost](https://live.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/basic_stream_socket/async_read_some.html)
    akes two arguments: buffer and handler. The example shows you can use `boost::asio::buffer(data, size)`
    This function doesn't guarantee reading the full buffer; use `async_read()` if you want that ~~No problems encountered for now~~
2. `boost::asio::buffer(date_, max_length)` [Boost](https://live.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/buffer.html)
    Just refer to the example in `async_read_some()`

### console.cpp
No `[Usage]` because this is called by `http_server` through `panel.cgi`

#### main
Entering this part means it's time to display `console.cgi` (i.e., the form has been filled and now we want to see the execution result)
1. First, retrieve the environment variables (they are set by `http_server`)
2. Extract the environment variable `"QUERY_STRING"`, and do parsing according to the spec ~~string processing~~, and fill each `UserInfo` accordingly
3. Output the contents of `sample_console.cgi`
4. Call `make_shared<Client>(io_context, user.first)->start();` to start the connection process

#### class Client
```=cpp
class Client : public enable_shared_from_this<Client> {}
```
1. `do_resolve()` : Convert domain name to IP [Boost](https://live.boost.org/doc/libs/1_37_0/doc/html/boost_asio/reference/ip__basic_resolver/async_resolve/overload1.html)
    ```=cpp
    void do_resolve() {
        resolver_.async_resolve(
            UserInfoMap[index_], to_string(UserInfoMap[index_].port), [this, self](boost::system::error_code ec, tcp::resolver::results_type result) {
                if (!ec) {
                    memset(data_, '\n', sizeof(data_));
                    endpoint_ = result;
                    do_connect();
                } else {
                    socket_.close();
                }
            }
        );
    }
    ```
2. `do_connect()` : Connect using the IP address [Boost](https://www.boost.org/doc/libs/1_88_0/doc/html/boost_asio/reference/async_connect.html)
    ```
    void do_connect() {
        auto self(shared_from_this());
        boost::asio::async_connect(
            socket_, enpoint_, [this, self](boost::system::error_code ec, tcp::endpoint ed) {
                if (!ec) {
                    memset(data_, '\0', sizeof(data_));
                    in.open("./test_case/" + UserInfo[index_].fileName);
                    if (!in.is_open()) {
                        socket_.close();
                    }
                    do_read();
                } else {
                    socket_close();
                }
            }
        );
    }
    ```
3. `do_read()`, `do_write()`
    3-1. Ouptut contain `"% "`, need to receive command -> `do_write()`
    3-2. Output do not contain `"% "`, need to receive exe result -> `do_read()`
    ```=cpp
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec && length) {
                    string output_msg = string(data_);
                    output(output_msg, "result");
                    if (output_msg.find("% ") == string::npos) {
                        do_read();
                    } else {
                        do_write();
                    }
                } else if (!length) {
                    socket_.close();
                }
            }
        );
    }
    
    void do_write() {
        auto self(shared_from_this());
        string input_cmd;
        getline(in, input_cmd);
        output(nput_cmd, "command");
        boost::asio::async_write(
            socket_, boost::asio::buffer(input_cmd, input_cmd,size()), [this, self](boost::system::error_code ec, size_t) {
                if (!ec) {
                    do_read();
                }
            }
        );
    }
    ```
    [Boost](https://live.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/async_write/overload1.html)
    The counterpart of `async_read_some()`

## Part 2 Windows Version
(Spec) Merge `http_server` and `console.cgi`, then run it on Windows 10/11

#### main
Same as `http_server`

#### class server
Same as `http_server`, responsible for handling `accept` and creating `class session`

#### class session
The parsing ~~string processing~~ part is the same as in `http_server`  
:::info
Differences in flow:  
Windows:
1. When receiving `/panel.cgi`, directly print out the full HTML; no need to fork -> exec  
2. After setting the environment, start a `thread` to execute `console(socket, environment)`, and the original recursion continues `do_read()`
Linux:
1. fork()
2. dup()
3. exe()
:::

#### console
`void console(tcp::socket &socket_, map<string, string> &env);`
Does the same thing as `main()` in Part 1’s `console.cpp` (string processing, printing `console.cgi`)  
**It doesn’t print using cout anymore**, now use the following function to write the output content,  
because this is all in one program and we can’t use dup to redirect output.
```
boost::asio::write(socket_, boost::assio::buffer(data))
```

#### class Client
Similar to `console.cpp`, The biggest difference is that output is no longer using cout

## Q&A
Yeah, Q&A was 12%, 3 questions, and I couldn’t answer one...
1. They asked what the `boost::asio::io_context io_context` in the `main` of `http_server` is used for..?
    (~~Damn, I copied that from `echo_server`, how would I know what it does~~) It’s for handling interaction with the OS. There’s no way a networking app doesn’t interact with the OS, right? So we just pass everything to this to handle.

2. They asked when the `handler` of some boost::... function is executed (~~Honestly I didn’t really understand the question~~)
    So I just said: It gets executed after the function it’s passed to finishes running. (But the TA didn’t seem satisfied...) So I added: It will return an `error_code` (TA seemed pleased once I said that keyword?)

3. They asked how to convert a Host Name to an IP?
    Just use `async_resolve`, and what it does is... DNS (Domain Name System)

## Demo
(Problem I found on other note in github: use `async_wait` to delay each command by one second. Only managed to write this thanks to GPT, couldn’t understand the API docs at all)

Third Problem: Delayed Command (CGI)
```=cpp
if (output_msg.find("% ") == string::npos) {
    do_read();
} else {
    // Delay 1 second before calling do_write()
    auto timer = make_shared<boost::asio::steady_time>(socket_.get_executor(), std::chrono::seconds(1));
    timer->async_waint(
        [this, self, time](const boost::system::error_code& ec) {
            if (!ec) {
                do_write();
            }
        }
    );
}
```

### Probably the Fourth Question  
(From what senior ~~Wu Xin-Bao~~ posted: when `boost::system::error_code& ec` might indicate an error)  
Uhh... I guess just output the required format? I don’t even know what the question was?


### First Question
(Sneak peek at a previous question: if the REQUEST is not `panel.cgi`, return 403)
Only accept requests for `/panel.cgi`; for other requests, return `HTTP1.1 403 Forbidden`  
I referred to a previous code that did it like this  
(though this would break the second part when switching to `console.cgi`, made me think the old question was wrong)  
In the `exec` step of `http_server.cpp`,  
add a check: if `REQUEST_URI` doesn’t contain `console.cgi`, then output (to STDOUT) `HTTP/1.1 403 Forbidden\r\n` back

### Second Question
This is the one I drew:
For each session in `console.cgi`, after receiving the first shell prompt (`%`), automatically send the `who` command to both the web page and `np_single_golden`  
How to do?
In `console.cpp`, inside `class Client`, add a `bool isFirst(true)`  
Then inside `do_write()`, if it’s the first time, directly change `input_cmd` to `who\n` (make sure to include `\n`), and set `isFirst` to `false`
