// C++ 基本工具：输出、字符串、字符串流、线程等。
#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const int BUFFER_SIZE = 4096;

std::atomic<int> active_threads{0};
std::mutex cout_mutex;

void log_message(const std::string& message) {
    // 多个 thread 可能同时打印，所以用 mutex 保护 cout。
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << message << std::endl;
}

struct ClientThreadLogger {
    ClientThreadLogger() {
        int count = ++active_threads;
        log_message("Client connected. Active threads: " + std::to_string(count));
    }

    ~ClientThreadLogger() {
        int count = --active_threads;
        log_message("Client disconnected. Active threads: " + std::to_string(count));
    }
};

bool send_all(int fd, const char* data, size_t length) {
    size_t sent_total = 0;
    while (sent_total < length) {
        ssize_t sent = send(fd, data + sent_total, length - sent_total, 0);
        if (sent <= 0) {
            return false;
        }
        sent_total += sent;
    }

    return true;
}

void send_error(int client_fd, int code, const std::string& message) {
    // 构造一个简单的 HTTP 错误响应，例如 400、501、502。
    std::string body = "<html><body><h1>" + std::to_string(code) + " " +
                       message + "</h1></body></html>\n";

    std::ostringstream response;
    response << "HTTP/1.0 " << code << " " << message << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    std::string text = response.str();
    send_all(client_fd, text.c_str(), text.size());
}

bool read_request(int client_fd, std::string& request) {
    char buffer[BUFFER_SIZE];

    // recv() 从 client socket 读取数据。
    // TCP 是字节流，一次 recv() 不一定能读到完整 HTTP request。
    // HTTP header 用空行结束，也就是 \r\n\r\n，少数情况也可能是 \n\n。
    while (request.find("\r\n\r\n") == std::string::npos &&
           request.find("\n\n") == std::string::npos) {
        ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            return false;
        }

        request.append(buffer, received);

        // 简单限制 header 大小，避免坏客户端一直发数据。
        // 这里 65536 就相当于本实验里的 MAX_HEADER_SIZE。
        if (request.size() > 65536) {
            return false;
        }
    }

    return true;
}

bool parse_url(const std::string& url, std::string& host, std::string& port, std::string& path) {
    std::string prefix = "http://";
    if (url.substr(0, prefix.size()) != prefix) {
        return false;
    }

    // 去掉开头的 http://，剩下 hostname[:port]/path。
    std::string rest = url.substr(prefix.size());
    if (rest.empty()) {
        return false;
    }

    // 用第一个 / 把 host[:port] 和 path 分开。
    size_t slash_pos = rest.find('/');
    std::string host_and_port;

    if (slash_pos == std::string::npos) {
        host_and_port = rest;
        // 如果 URL 没有写 path，默认访问根目录 /。
        path = "/";
    } else {
        host_and_port = rest.substr(0, slash_pos);
        path = rest.substr(slash_pos);
    }

    if (host_and_port.empty()) {
        return false;
    }

    // 如果有冒号，冒号后面是端口；否则 HTTP 默认端口是 80。
    size_t colon_pos = host_and_port.find(':');
    if (colon_pos == std::string::npos) {
        host = host_and_port;
        port = "80";
    } else {
        host = host_and_port.substr(0, colon_pos);
        port = host_and_port.substr(colon_pos + 1);
    }

    return !host.empty() && !port.empty();
}

int connect_to_server(const std::string& host, const std::string& port) {
    // 到这里以后，proxy 要像 client 一样连接真正的 web server。
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* results;
    // getaddrinfo 把主机名和端口转换成 connect() 可以使用的地址信息。
    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        return -1;
    }

    int server_fd = -1;

    for (addrinfo* p = results; p != nullptr; p = p->ai_next) {
        // 创建一个用于连接远程 server 的 socket。
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd == -1) {
            continue;
        }

        // connect 成功后，server_fd 就连接到了真正的网站。
        if (connect(server_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(server_fd);
        server_fd = -1;
    }

    freeaddrinfo(results);
    return server_fd;
}

void handle_client(int client_fd) {
    // 这个对象创建时记录连接，函数退出时自动记录断开。
    ClientThreadLogger thread_logger;

    // 1. 读取浏览器或 telnet 发来的 HTTP request。
    std::string request;
    if (!read_request(client_fd, request)) {
        send_error(client_fd, 400, "Bad Request");
        close(client_fd);
        return;
    }

    // 2. 取出第一行 request line。
    // 正常格式类似：GET http://example.com/path HTTP/1.0
    std::istringstream request_stream(request);
    std::string request_line;
    std::getline(request_stream, request_line);
    // HTTP 每行通常用 \r\n 结束，getline 去掉 \n 后可能留下 \r。
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    // 3. 把 request line 分成 method、url、version 三部分。
    std::istringstream line_stream(request_line);
    std::string method;
    std::string url;
    std::string version;
    std::string extra;

    if (!(line_stream >> method >> url >> version) || (line_stream >> extra)) {
        send_error(client_fd, 400, "Bad Request");
        close(client_fd);
        return;
    }

    // 本实验只实现 GET，其他方法一律返回 501。
    if (method != "GET") {
        send_error(client_fd, 501, "Not Implemented");
        close(client_fd);
        return;
    }

    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        send_error(client_fd, 400, "Bad Request");
        close(client_fd);
        return;
    }

    // 4. 解析 absolute URL，得到 host、port、path。
    std::string host;
    std::string port;
    std::string path;
    if (!parse_url(url, host, port, path)) {
        send_error(client_fd, 400, "Bad Request");
        close(client_fd);
        return;
    }

    log_message("Requested URL: " + url);
    log_message("Host: " + host);
    log_message("Port: " + port);
    log_message("Path: " + path);

    // 5. 连接真正的远程 web server。
    int server_fd = connect_to_server(host, port);
    if (server_fd == -1) {
        send_error(client_fd, 502, "Bad Gateway");
        close(client_fd);
        return;
    }

    // 6. 浏览器发给 proxy 的是 absolute URL。
    // 但真正的 web server 需要 relative path，例如 GET /index.html HTTP/1.0。
    // 这里强制使用 HTTP/1.0 和 Connection: close，方便读到 server 关闭连接为止。
    std::ostringstream forward;
    forward << "GET " << path << " HTTP/1.0\r\n";
    forward << "Host: " << host << "\r\n";
    forward << "Connection: close\r\n";
    forward << "\r\n";

    std::string forward_request = forward.str();
    if (!send_all(server_fd, forward_request.c_str(), forward_request.size())) {
        send_error(client_fd, 502, "Bad Gateway");
        close(server_fd);
        close(client_fd);
        return;
    }

    // 7. 从远程 server 读取 response，并立刻转发回原来的 client。
    // 一直循环，直到远程 server 关闭连接或出错。
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t received = recv(server_fd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }

        if (!send_all(client_fd, buffer, received)) {
            break;
        }
    }

    // 8. 请求处理结束，关闭两个 socket。
    close(server_fd);
    close(client_fd);
}

int create_server_socket(const char* port) {
    // 创建监听 socket。这个 socket 用来等待浏览器连接 proxy。
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* results;
    // getaddrinfo 准备本机地址信息，因为这里是 bind 本地端口。
    int status = getaddrinfo(nullptr, port, &hints, &results);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        return -1;
    }

    int listen_fd = -1;

    for (addrinfo* p = results; p != nullptr; p = p->ai_next) {
        // socket() 创建一个 TCP socket。
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd == -1) {
            continue;
        }

        // SO_REUSEADDR 让程序重启后更容易重新使用同一个端口。
        int yes = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        // bind() 把 socket 绑定到命令行传入的 port。
        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(results);

    if (listen_fd == -1) {
        perror("bind");
        return -1;
    }

    // listen() 开始监听客户端连接。
    // 20 是 backlog，表示内核可以排队等待的连接数量。
    if (listen(listen_fd, 20) == -1) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int main(int argc, char* argv[]) {
    // argc/argv 是命令行参数。本程序需要：./proxy <port>
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    // 如果 client 提前断开，send() 可能触发 SIGPIPE。
    // 忽略它可以避免整个 proxy 程序直接退出。
    signal(SIGPIPE, SIG_IGN);

    // 创建监听 socket，监听用户指定的端口。
    int listen_fd = create_server_socket(argv[1]);
    if (listen_fd == -1) {
        return 1;
    }

    log_message("Proxy listening on port " + std::string(argv[1]));

    while (true) {
        sockaddr_storage client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // accept() 等待一个新的浏览器/telnet client 连接。
        int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // 每个 client 用一个 thread 处理，所以可以同时处理多个连接。
        std::thread worker(handle_client, client_fd);
        // detach 表示主线程不等待这个 worker，worker 自己运行到结束。
        worker.detach();
    }

    close(listen_fd);
    return 0;
}
