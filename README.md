# Project 4

## Part 1 Linux Version

### http_server.cpp
`[Usage]: ./http_server [port]`
(根據Spec) 這裡要做的事情是
1. 處理 HTTP 的 URL (e.g., `http://127.0.0.1:7001/panel.cgi`)
2. 把 URL 裡面的 `something.cgi` 叫起來執行
3. 根據 URL 把環境變數弄好
4. 用 `<boost/asio.hpp>` 完成

#### main
(照搬`extra_files/echo_server.cpp`裡面的`main`，沒有變)
就把`class server`叫起來而已

#### class server
(照搬`extra_files/echo_server.cpp`裡面的`server`，沒有變)
就負責accept東西而已，`constructor`也只有call `do_accept()`
然後`do_accept()`接收到東西後也就開一個`class session`出來處理，接著recursion繼續accept
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
這面這段function的順序是
1. 進入 `void do_accept()` 
2. 進入 `acceptor_.aync_accept()` 吃 1 個 argument
    2-1. 一個 lambda function `[this](boost::system::error ec, tcp::socket socket)` (Document叫這個handler)
3. **先處理完`aync_accept()`之後，接著進 lambda function**
4. lambda function 會把創一個 `class session` 繼續處理

那 `make_shared<session>(move(socket))->start();` 是什麼?
關於 `make_shared` [StackOverfolw](https://stackoverflow.com/questions/50312423/boostmake-shared-fail-but-stdmake-shared-work), [Boost](https://www.boost.org/doc/libs/1_44_0/libs/smart_ptr/make_shared.html) 反正我要很粗暴的理解為就是一個 shared pointer 啦

#### class session
(架構也是從`extra_file/echo_server.cpp`搬過來的，主要增加`do_read()`裡面的事情，然後沒有`do_write()`，因為要直接執行`some.cgi`)
繼續把 spec 要求處理完的部分
```=cpp
class session: public enable_shared_from_this<session> {};
```
(不管就粗暴解釋為跟上面的`make_shared`是一組的)
```=cpp
void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(
        boost::asio::buffer(date_, max_length), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                fork()
                child_1: 填好environment
                    // 講好一點 parsing, 講難聽一點字串處理
                child_2: dup 給 client
                child_3: exec `cgi`
                    3-1. 輸出 http protocal (結尾"\r\n") 過去
                    3-2. 執行 
                parent: 把 socket_ 關掉 ， 讓 child 連著就好
            }
        }
    );
}
```
1. ~~字串處理~~Parsing就照Spec上要求拆開
2. `dup(socket_.native_handle(), FD)`恩對[Boost](https://live.boost.org/doc/libs/1_87_0/doc/html/boost_asio/reference/basic_stream_socket/native_handle.html)
3. `exec .cgi`
    3-1. 把URL抓出來，如果沒有`.cgi`就丟`"HTTP/1.1 403 Forbiden\r\n"`回去
    3-2. 有`.cgi` 就丟`"HTTP/1.1 200 OK\r\nServer: http_server\r\n"`
    3-3. 執行`.cgi` (`panel.cgi`TA給的, `console.cgi`下面產生的，理論上`panel.cgi` run按下去後URL會轉成`console.cgi`的)
    
Boost Function Note:
1. `socket_.async_read_some()` [Boost](https://live.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/basic_stream_socket/async_read_some.html)
    吃兩個東西，buffer和handler，下面有example說single buffer可以用`boost::asio::buffer(data, size)`
    這個function不一定會全部吃完buffer，想要保證全部吃完就用`async_read()` ~~目前沒有遇到問題~~
2. `boost::asio::buffer(date_, max_length)` [Boost](https://live.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/buffer.html)
    直接參考`async_read_some()`裡面的好了

### console.cpp
沒有`[Usege]` 因為這是`panel.cgi`會透過`http_server`叫起來的東西

#### main
會進到這個地方，表示要顯示`console.cgi`了(也就是表格填完要看執行結果了)
1. 先把環境變數撈出來(`http_server`會設定好)
2. 把環境變數`"QUERY_STRING"`拿出來依照spec做parsing~~字串處理~~，把每個 UserInfo 給塞好
3. 把 `sample_console.cgi` 的東西拿出來輸出
4. 叫出`make_shared<Client>(io_context, user.first)->start();` 開始跑連線的部分

#### class Client
```=cpp
class Client : public enable_shared_from_this<Client> {}
```
1. `do_resolve()` : 把 domain name 轉成 IP [Boost](https://live.boost.org/doc/libs/1_37_0/doc/html/boost_asio/reference/ip__basic_resolver/async_resolve/overload1.html)
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
2. `do_connect()` : 依照 IP 連線 [Boost](https://www.boost.org/doc/libs/1_88_0/doc/html/boost_asio/reference/async_connect.html)
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
    3-1. 輸出有 `"% "` 要收command -> `do_write()`
    3-2. 輸出沒有 `"% "` 要收結果 -> `do_read()`
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
    跟`async_read_some()`對稱的東西

## Part 2 Windows Version
(Spec) 把`http_server` 和 `console.cgi` 融在一起然後丟到 Windows10/11

#### main
跟`http_server`是一樣的

#### class server
跟`http_server`是一樣的，負責接`accept`然後開`class session`

#### class session
parsing~~字串處理~~的部分和`http_server`是一樣的
:::info
流程不一樣的地方
Windows:
1. 原本收到`/panel.cgi`直接把html整個印出來，不用去fork -> exe
2. 接好environment之後，開一個`thread`去執行`console(socket, environment)`，，原本的recursion繼續`do_read()`
Linux:
1. fork()
2. dup()
3. exe()
:::

#### console
`void console(tcp::socket &socket_, map<string, string> &env);`
和 Part 1 `console.cpp`裡面的 `main()` 做一樣的事情 (字串處理，把console.cgi印好)
**不是印好，現在不是用cout了**，用下面的function把要輸出的內容寫出去，因為現在是在同一支程式，沒辦法用dup之後輸出。
```
boost::asio::write(socket_, boost::assio::buffer(data))
```

#### class Client
和`console.cpp` 大致一樣， 最大差別就是輸出一樣不是用cout了

## Demo
等之後再說
```=cpp
/*
if (output_msg.find("% ") == string::npos) {
    do_read();
} else {
    // Delay 1 second before calling do_write()
    auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor(), std::chrono::seconds(1));
    auto self = shared_from_this();
    timer->async_wait([this, self, timer](const boost::system::error_code& ec) {
        if (!ec) {
            do_write();
        }
    });
}
*/
```