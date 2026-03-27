#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main(int argc, char *argv[])
{
    // Check command line arguments
    if (argc != 3)
    {
        cerr << "Usage: ./server -t <port-number>" << endl;
        return 1;
    }

    string mode = argv[1];
    if (mode != "-t")
    {
        cerr << "Error: this version only supports TCP mode (-t)." << endl;
        return 1;
    }

    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535)
    {
        cerr << "Error: invalid port number." << endl;
        return 1;
    }

    // 1. Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket failed");
        return 1;
    }

    // Optional but helpful: allow quick reuse of port
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server_socket);
        return 1;
    }

    // 2. Build server address
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on all local interfaces
    server_addr.sin_port = htons(port);

    // 3. Bind
    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_socket);
        return 1;
    }

    // 4. Listen
    if (listen(server_socket, 5) < 0)
    {
        perror("listen failed");
        close(server_socket);
        return 1;
    }

    cout << "Serial Server on host 0.0.0.0 is listening on port " << port << endl;
    cout << endl;
    cout << "Serial Server starting, listening on port " << port << endl;
    cout << endl;

    // 5. Accept one client
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int comm_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);
    if (comm_socket < 0)
    {
        perror("accept failed");
        close(server_socket);
        return 1;
    }

    cout << "Received connection request from " << inet_ntoa(client_addr.sin_addr) << endl;
    cout << endl;
    cout << "***********************************************************" << endl;
    cout << endl;
    cout << "Now listening for incoming messages..." << endl;
    cout << endl;

    // 6. Receive messages
    char buffer[1024];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(comm_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received < 0)
        {
            perror("recv failed");
            close(comm_socket);
            close(server_socket);
            return 1;
        }

        if (bytes_received == 0)
        {
            cout << "Client disconnected." << endl;
            break;
        }

        buffer[bytes_received] = '\0';

        cout << "Received the following message from client:" << endl;
        cout << endl;
        cout << "\"" << buffer << "\"" << endl;
        cout << endl;

        if (strcmp(buffer, ";;;") == 0)
        {
            cout << "Client finished." << endl;
            break;
        }
    }

    close(comm_socket);
    close(server_socket);

    return 0;
}