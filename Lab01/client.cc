#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;
string mode;
int port;
char* address;

bool send_all(int sockfd, const char *data, int total_len)
{
    int sent_total = 0;

    while (sent_total < total_len)
    {
        int sent_now = send(sockfd, data + sent_total, total_len - sent_total, 0);

        if (sent_now <= 0)
        {
            return false;
        }

        sent_total += sent_now;
    }

    return true;
}
bool resolve_host(const char *host, struct sockaddr_in &server_addr)
{
    struct hostent *server = gethostbyname(host);
    if (server == nullptr)
    {
        return false;
    }

    if (server->h_addrtype != AF_INET)
    {
        return false;
    }

    memcpy(&server_addr.sin_addr, server->h_addr_list[0], server->h_length);
    return true;
}

int TCP_client(){
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        cerr << "socket failed" << endl;
        return -1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // 清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (!resolve_host(address, server_addr))
    {
        cerr << "cannot resolve hostname or IP address" << endl;
        if (close(client_socket) < 0)
        {
            cerr << "close failed" << endl;
        }
        return -1;
    }

    int conn = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (conn < 0){
        cerr << "connect faild" << endl;
        return -1;
    }
    cout << "Client has requested to start connection with host "
         << address << " on port " << port << endl
         << endl;

    cout << "***********************************************************" << endl
         << endl;

    cout << "Connection established, now waiting for user input..."
         << endl
         << endl;
    string message;
    cout << "prompt> ";
    getline(cin, message);

    while (true){
        if (message == ";;;")
        {
            cout << endl
                 << "Sending message to Server..." << endl
                 << endl;

            if (!send_all(client_socket, message.c_str(), message.length()))
            {
                cerr << "send failed" << endl;
            }

            cout << "User entered sentinel of \";;;\", now stopping client"
                 << endl
                 << endl;
            cout << "***********************************************************" << endl
                 << endl;
            cout << "Attempting to shut down client sockets and other streams"
                 << endl
                 << endl;
            break;
        }

        cout << endl
             << "Sending message to Server..." << endl
             << endl;
        if (!send_all(client_socket, message.c_str(), message.length()))
        {
            cerr << "send failed" << endl;
            if (close(client_socket) < 0)
            {
                cerr << "close failed" << endl;
            }
            return -1;
        }
        char buffer[1024];
        int data = recv(client_socket, buffer, sizeof(buffer), 0);
        if (data <= 0)
        {
            cout << "Server disconnected." << endl;
            break;
        }
        buffer[data] = '\0';
        cout << "Received response from server of" << endl
             << endl;
        cout << "\"" << buffer << "\"" << endl
             << endl;
        cout << "prompt> ";
        getline(cin, message);
    }

    if (close(client_socket) < 0)
    {
        cerr << "close failed" << endl;
        return -1;
    }
    cout << "Shut down successful... goodbye" << endl;
    return 0;
}

int UDP_client(){
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        cerr << "socket failed" << endl;
        return -1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // 清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (!resolve_host(address, server_addr))
    {
        cerr << "cannot resolve hostname or IP address" << endl;
        if (close(client_socket) < 0)
        {
            cerr << "close failed" << endl;
        }
        return -1;
    }
    cout << "Client has opened UDP socket to start communication with host "
         << address << " on port " << port << endl
         << endl;

    cout << "***********************************************************" << endl
         << endl;
    cout << "Now waiting for user input..." << endl
         << endl;

    string message;
    while (true)
    {
        cout << "prompt> ";
        getline(cin, message);
        cout << endl
             << "Sending message to Server..." << endl
             << endl;

        if (message == ";;;")
        {
            cout << "User entered sentinel of \";;;\", now stopping client"
                 << endl
                 << endl;
            cout << "***********************************************************" << endl
                 << endl;
            cout << "Attempting to shut down client sockets and other streams"
                 << endl
                 << endl;
            break;
        }
        if (sendto(client_socket, message.c_str(), message.length(), 0, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cerr << "send faild" << endl;
        }
    }
    if (close(client_socket) < 0)
    {
        cerr << "close failed" << endl;
        return -1;
    }
    cout << "Shut down successful... goodbye" << endl;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: ./server -t <port-number>" << endl;
        return 1;
    }

    mode = argv[1];
    if (mode != "-t" && mode != "-u")
    {
        cerr << "You need a right model -t or -u\n";
        return 1;
    }

    address = argv[2];

    port = atoi(argv[3]);
    if (port < 0 || port > 65535)
    {
        cerr << "bad port number\n";
        return 1;
    }

    if (mode == "-t")
    {
        TCP_client();
    }
    if (mode == "-u")
    {
        UDP_client();
    }
}