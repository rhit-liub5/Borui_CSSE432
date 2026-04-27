#include "common.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

bool send_all(int sockfd, const char *data, size_t len)
{
    size_t sent_total = 0;
    while (sent_total < len)
    {
        ssize_t sent_now = send(sockfd, data + sent_total, len - sent_total, 0);
        if (sent_now <= 0)
        {
            return false;
        }
        sent_total += (size_t)sent_now;
    }
    return true;
}

bool recv_all(int sockfd, char *data, size_t len)
{
    size_t received_total = 0;
    while (received_total < len)
    {
        ssize_t received_now = recv(sockfd, data + received_total, len - received_total, 0);
        if (received_now <= 0)
        {
            return false;
        }
        received_total += (size_t)received_now;
    }
    return true;
}

bool send_line(int sockfd, const string &line)
{
    string msg = line + "\n";
    return send_all(sockfd, msg.c_str(), msg.size());
}

bool recv_line(int sockfd, string &line)
{
    line.clear();
    char ch;
    while (true)
    {
        ssize_t n = recv(sockfd, &ch, 1, 0);
        if (n <= 0)
        {
            return false;
        }
        if (ch == '\n')
        {
            return true;
        }
        if (ch != '\r')
        {
            line += ch;
        }
    }
}

int connect_to_host(const string &host, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        hostent *server = gethostbyname(host.c_str());
        if (server == nullptr || server->h_addrtype != AF_INET)
        {
            close(sockfd);
            return -1;
        }
        memcpy(&addr.sin_addr, server->h_addr_list[0], server->h_length);
    }

    if (::connect(sockfd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int create_listen_socket(const string &host, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return -1;
    }

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (host == "0.0.0.0")
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        close(sockfd);
        return -1;
    }

    if (::bind(sockfd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sockfd);
        return -1;
    }
    if (::listen(sockfd, 64) < 0)
    {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

vector<string> split_words(const string &line)
{
    vector<string> out;
    string item;
    stringstream ss(line);
    while (ss >> item)
    {
        out.push_back(item);
    }
    return out;
}

string join_ints(const vector<int> &values, char sep)
{
    string out;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            out += sep;
        }
        out += to_string(values[i]);
    }
    return out;
}

vector<int> split_ints(const string &text, char sep)
{
    vector<int> values;
    string item;
    stringstream ss(text);
    while (getline(ss, item, sep))
    {
        if (!item.empty())
        {
            values.push_back(stoi(item));
        }
    }
    return values;
}

string basename_of(const string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}
