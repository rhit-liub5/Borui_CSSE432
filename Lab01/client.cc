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

int TCP_client(){
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // 清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(address);

    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    string message;
    cout << "Enter message to send to server: ";
    getline(cin, message);

    while (true){
        if (message == ";;;"){
            send(client_socket, message.c_str(), message.length(), 0);
            break;
        }
        send(client_socket, message.c_str(), message.length(), 0);
        // ssize_t send(int sockfd, const void *buf, size_t len, int flags);d
        //不能用string发送，用message.c_str()把string转换成char*
        char buffer[1024];
        int data = recv(client_socket, buffer, sizeof(buffer), 0);
        if (data <= 0)
        {
            cout << "Server disconnected." << endl;
            break;
        }
        buffer[data] = '\0';
        cout << "Received from server: " << buffer << endl;
        cout << "Enter message to send to server: ";
        getline(cin, message);
    }

    close(client_socket);
    return 0;
}

int UDP_client(){
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // 清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(address);

    string message;
    while (true)
    {
        cout << "Enter message to send to server: ";
        getline(cin, message);
        if (message == ";;;")
        {
            sendto(client_socket, message.c_str(), message.length(), 0, (sockaddr *)&server_addr,sizeof(server_addr));
            break;
        }
        sendto(client_socket, message.c_str(), message.length(), 0, (sockaddr *)&server_addr, sizeof(server_addr));
    }
    close(client_socket);
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
    }

    address = argv[2];

    port = atoi(argv[3]);
    if (port < 0 || port > 65535)
    {
        cerr << "bad port number\n";
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