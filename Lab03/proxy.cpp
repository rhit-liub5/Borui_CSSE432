#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

constexpr int BACKLOG = 20;
constexpr size_t BUFFER_SIZE = 4096;
constexpr size_t MAX_HEADER_SIZE = 64 * 1024;

struct ParsedRequest {
    std::string method;
    std::string url;
    std::string version;
    std::string host;
    std::string port;
    std::string path;
    std::vector<std::string> headers;
};

bool send_all(int socket_fd, const char* data, size_t length) {
    size_t total_sent = 0;

    while (total_sent < length) {
        ssize_t sent = send(socket_fd, data + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += static_cast<size_t>(sent);
    }

    return true;
}

bool send_all(int socket_fd, const std::string& data) {
    return send_all(socket_fd, data.c_str(), data.size());
}

void send_error_response(int client_socket, int status_code, const std::string& reason) {
    std::ostringstream body;
    body << "<html><body><h1>" << status_code << " " << reason << "</h1></body></html>\n";

    std::ostringstream response;
    response << "HTTP/1.0 " << status_code << " " << reason << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.str().size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body.str();

    send_all(client_socket, response.str());
}

std::string trim_trailing_cr(const std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        return line.substr(0, line.size() - 1);
    }
    return line;
}

std::string lowercase(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

bool starts_with_case_insensitive(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }

    return lowercase(text.substr(0, prefix.size())) == lowercase(prefix);
}

bool read_http_request(int client_socket, std::string& request) {
    char buffer[BUFFER_SIZE];

    while (request.find("\r\n\r\n") == std::string::npos &&
           request.find("\n\n") == std::string::npos) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            return false;
        }

        request.append(buffer, static_cast<size_t>(bytes_received));
        if (request.size() > MAX_HEADER_SIZE) {
            return false;
        }
    }

    return true;
}

bool parse_request_line(const std::string& request_line,
                        std::string& method,
                        std::string& url,
                        std::string& version) {
    std::istringstream line_stream(request_line);
    std::string extra;

    if (!(line_stream >> method >> url >> version)) {
        return false;
    }

    // A valid request line has exactly three whitespace-separated parts.
    if (line_stream >> extra) {
        return false;
    }

    return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool parse_url(const std::string& url,
               std::string& host,
               std::string& port,
               std::string& path) {
    const std::string prefix = "http://";
    if (!starts_with_case_insensitive(url, prefix)) {
        return false;
    }

    std::string rest = url.substr(prefix.size());
    if (rest.empty()) {
        return false;
    }

    size_t path_start = rest.find('/');
    std::string host_port;

    if (path_start == std::string::npos) {
        host_port = rest;
        path = "/";
    } else {
        host_port = rest.substr(0, path_start);
        path = rest.substr(path_start);
        if (path.empty()) {
            path = "/";
        }
    }

    if (host_port.empty()) {
        return false;
    }

    size_t colon = host_port.rfind(':');
    if (colon == std::string::npos) {
        host = host_port;
        port = "80";
    } else {
        host = host_port.substr(0, colon);
        port = host_port.substr(colon + 1);
        if (host.empty() || port.empty()) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> parse_headers(const std::string& request) {
    std::vector<std::string> headers;
    std::istringstream request_stream(request);
    std::string line;

    // Skip the request line.
    std::getline(request_stream, line);

    while (std::getline(request_stream, line)) {
        line = trim_trailing_cr(line);
        if (line.empty()) {
            break;
        }
        headers.push_back(line);
    }

    return headers;
}

int create_listening_socket(const char* port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* results = nullptr;
    int status = getaddrinfo(nullptr, port, &hints, &results);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << '\n';
        return -1;
    }

    int listen_socket = -1;
    for (addrinfo* current = results; current != nullptr; current = current->ai_next) {
        listen_socket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (listen_socket == -1) {
            continue;
        }

        int reuse = 1;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (bind(listen_socket, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        close(listen_socket);
        listen_socket = -1;
    }

    freeaddrinfo(results);

    if (listen_socket == -1) {
        perror("bind");
        return -1;
    }

    if (listen(listen_socket, BACKLOG) == -1) {
        perror("listen");
        close(listen_socket);
        return -1;
    }

    return listen_socket;
}

int connect_to_server(const std::string& host, const std::string& port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* results = nullptr;
    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
    if (status != 0) {
        std::cerr << "getaddrinfo for remote host failed: " << gai_strerror(status) << '\n';
        return -1;
    }

    int server_socket = -1;
    for (addrinfo* current = results; current != nullptr; current = current->ai_next) {
        server_socket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (server_socket == -1) {
            continue;
        }

        if (connect(server_socket, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        close(server_socket);
        server_socket = -1;
    }

    freeaddrinfo(results);
    return server_socket;
}

std::string build_forward_request(const ParsedRequest& request) {
    std::ostringstream forward;

    forward << "GET " << request.path << " HTTP/1.0\r\n";
    forward << "Host: " << request.host << "\r\n";
    forward << "Connection: close\r\n";

    for (const std::string& header : request.headers) {
        if (starts_with_case_insensitive(header, "Host:") ||
            starts_with_case_insensitive(header, "Connection:") ||
            starts_with_case_insensitive(header, "Proxy-Connection:")) {
            continue;
        }
        forward << header << "\r\n";
    }

    forward << "\r\n";
    return forward.str();
}

bool parse_client_request(const std::string& raw_request, ParsedRequest& parsed) {
    std::istringstream request_stream(raw_request);
    std::string request_line;

    if (!std::getline(request_stream, request_line)) {
        return false;
    }
    request_line = trim_trailing_cr(request_line);

    if (!parse_request_line(request_line, parsed.method, parsed.url, parsed.version)) {
        return false;
    }

    parsed.headers = parse_headers(raw_request);
    return true;
}

void forward_response(int server_socket, int client_socket) {
    char buffer[BUFFER_SIZE];

    while (true) {
        ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        if (!send_all(client_socket, buffer, static_cast<size_t>(bytes_received))) {
            break;
        }
    }
}

void handle_client(int client_socket) {
    std::string raw_request;
    if (!read_http_request(client_socket, raw_request)) {
        send_error_response(client_socket, 400, "Bad Request");
        close(client_socket);
        return;
    }

    ParsedRequest request;
    if (!parse_client_request(raw_request, request)) {
        send_error_response(client_socket, 400, "Bad Request");
        close(client_socket);
        return;
    }

    if (request.method != "GET") {
        send_error_response(client_socket, 501, "Not Implemented");
        close(client_socket);
        return;
    }

    if (!parse_url(request.url, request.host, request.port, request.path)) {
        send_error_response(client_socket, 400, "Bad Request");
        close(client_socket);
        return;
    }

    int server_socket = connect_to_server(request.host, request.port);
    if (server_socket == -1) {
        send_error_response(client_socket, 502, "Bad Gateway");
        close(client_socket);
        return;
    }

    std::string forward_request = build_forward_request(request);
    if (send_all(server_socket, forward_request)) {
        forward_response(server_socket, client_socket);
    } else {
        send_error_response(client_socket, 502, "Bad Gateway");
    }

    close(server_socket);
    close(client_socket);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    // Avoid process termination if a browser closes while we are sending data.
    signal(SIGPIPE, SIG_IGN);

    int listen_socket = create_listening_socket(argv[1]);
    if (listen_socket == -1) {
        return 1;
    }

    std::cout << "Proxy listening on port " << argv[1] << '\n';

    while (true) {
        sockaddr_storage client_address{};
        socklen_t client_address_size = sizeof(client_address);

        int client_socket = accept(
            listen_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_size);

        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(listen_socket);
    return 0;
}
