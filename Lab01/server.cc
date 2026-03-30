#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>

using namespace std;
string mode;
int port;
int message_num = 0;

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

void TCP_server_thread(int comm_socket, int client_num){
    char buffer[1024];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        int incoming_data = recv(comm_socket,buffer,sizeof(buffer),0);
        if (incoming_data <= 0){
            cerr << "no data recive" << endl;
            break;
        }
        buffer[incoming_data] = '\0';
        if (string(buffer) == ";;;"){
            cout << "Received the following message from client:" << endl
                 << endl;
            cout << "\";;;\"" << endl
                 << endl;
            cout << "Client finished, now waiting to service another client..."
                 << endl
                 << endl;
            cout << "***********************************************************" << endl
                 << endl;
            break;
        }
        message_num++;

        cout << "Received the following message from client:" << endl
             << endl;
        cout << "\"" << buffer << "\"" << endl
             << endl;
        for (int i = 0; buffer[i]; i++)
        {
            buffer[i] = toupper(buffer[i]);
        }
        string response = to_string(message_num) + " " + string(buffer);
        cout << "Now sending message " << message_num
             << " back having changed the string to upper case..."
             << endl
             << endl;
        if (!send_all(comm_socket, response.c_str(), response.length()))
        {
            cerr << "send failed" << endl;
            break;
        }
    }
    if (close(comm_socket) < 0)
    {
        cerr << "close failed" << endl;
    }
}



int TCP_server(){
    char host_name[256];
    if (gethostname(host_name,256) == -1){
        cerr << "Error getting hostname" << endl;
        return -1;
    }

    hostent *host = gethostbyname(host_name);
    if (host == nullptr){
        cerr << "Error getting host information" << endl;
        return -1;
    }
    // struct hostent
    // {
    //     char *h_name;
    //     char **h_aliases;
    //     int h_addrtype;
    //     int h_length;
    //     char **h_addr_list;
    // };
    int i = 0;
    while (host->h_addr_list[i] != NULL){
        cout << "Host name: " << host->h_name << "\n" <<
                "Aliases: " << host->h_aliases[i] << "\n" <<
                "IP Address: " << inet_ntoa(*(struct in_addr *)host->h_addr_list[i]) << endl;
        i++;
    }
    //     host->h_addr_list[i] 不是字符串
    //     这个是关键。
    //     虽然它是 char *，但它指向的是 4 字节的二进制 IP 地址，不是字符串。
    //     例如：
    //     127.0.0.1 = 7F 00 00 01
    //     中间有 00，也就是 '\0'，所以 cout 会提前停止。
    //     所以你不能直接：
    //     cout<< host->h_addr_list[i];
    //     因为那不是字符串。
    //     必须把它解释成 IP 地址结构，然后转换： 
    //     char * → struct in_addr * → struct in_addr → inet_ntoa → string

    string host_ip = "";
    i = 0;
    while (host_ip == ""){
        if (strcmp(inet_ntoa(*(struct in_addr *)host->h_addr_list[i]),"127.0.1.1") == 0){
            host_ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[i]);
            break;
        }
        i++;
    }

    cout << "Host name: " << host->h_name << "\n"
         << "IP: " << host_ip << "\n"
         << "Port " << port << endl;

    int server_listening_socket = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
    if (server_listening_socket < 0)
    {
        cerr << "socket create error" << endl;
        return -1;
    }
    int opt = 1;
    setsockopt(server_listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 设置套接字选项，允许重用地址
    if (opt < 0)
    {
        cerr << "setsockopt failed" << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); //清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // struct sockaddr_in
    // {
    //     short sin_family;        // 地址类型 AF_INET
    //     unsigned short sin_port; // 端口号 用ntohs转换成Char*
    //     struct in_addr sin_addr; // IP地址 用inet_ntoa转换成Char*
    //     char sin_zero[8];        // 填充
    // };

    int bin = bind(server_listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // 绑定套接字到地址和端口
    //(struct sockaddr *)&server_addr 将server_addr转换成更加通用的sockaddr类型。bind只接受这个类型
    if (bin < 0){
        cerr << "bind failed" << endl;
        return -1; 
    }
    int lis = listen(server_listening_socket, 5); //开始监听连接，参数为最大连接数
    if (lis < 0){
        cerr << "listen failed" << endl;
        return -1;
    }

    cout << "Serial Server on host 0.0.0.0/0.0.0.0 is listening on port "
         << port << endl
         << endl;
    cout << "Serial Server starting, listening on port "
         << port << endl
         << endl;
    cout << "***********************************************************" << endl
         << endl;

    int client_num = 1;

    while (true){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // socklen_t 是一种专门用来表示 socket 地址长度的类型。
        // 本质上它其实就是一个整数类型（类似 int），只是专门用于 socket API。 
        int server_comm_socket = accept(server_listening_socket, (struct sockaddr *)&client_addr, &client_len);
        if (server_comm_socket < 0){
            cerr << "accept error" << endl;
            continue; // 接受连接失败，继续等待下一个连接
        }
        cout << "Received connection request from /"
             << inet_ntoa(client_addr.sin_addr) << endl
             << endl;

        cout << "***********************************************************" << endl
             << endl;

        cout << "Now listening for incoming messages..." << endl
             << endl;
        thread t(TCP_server_thread, server_comm_socket, client_num);
        t.detach();

        client_num++;
    }
    return 0;
}

void UDP_server_thread(char *buffer)
{
    cout << "Received from client " << "message: " << message_num << ": " << buffer << endl;
    for (int i = 0; buffer[i]; i++)
    {
        buffer[i] = toupper(buffer[i]);
    }
    cout << "print: " << buffer << endl;
    delete[] buffer;
}

int UDP_server(){
    char host_name[256];
    if (gethostname(host_name, 256) == -1)
    {
        cerr << "get hostname error" << endl;
        return -1;
    }

    hostent *host = gethostbyname(host_name);
    if (host == nullptr)
    {
        cerr << "Error getting host information" << endl;
        return -1;
    }
    // struct hostent
    // {
    //     char *h_name;
    //     char **h_aliases;
    //     int h_addrtype;
    //     int h_length;
    //     char **h_addr_list;
    // };
    int i = 0;
    while (host->h_addr_list[i] != NULL)
    {
        cout << "Host name: " << host->h_name << "\n"
             << "Aliases: " << host->h_aliases[i] << "\n"
             << "IP Address: " << inet_ntoa(*(struct in_addr *)host->h_addr_list[i]) << endl;
        i++;
    }
    //     host->h_addr_list[i] 不是字符串
    //     这个是关键。
    //     虽然它是 char *，但它指向的是 4 字节的二进制 IP 地址，不是字符串。
    //     例如：
    //     127.0.0.1 = 7F 00 00 01
    //     中间有 00，也就是 '\0'，所以 cout 会提前停止。
    //     所以你不能直接：
    //     cout<< host->h_addr_list[i];
    //     因为那不是字符串。
    //     必须把它解释成 IP 地址结构，然后转换：
    //     char * → struct in_addr * → struct in_addr → inet_ntoa → string

    string host_ip = "";
    i = 0;
    while (host_ip == "")
    {
        if (strcmp(inet_ntoa(*(struct in_addr *)host->h_addr_list[i]), "127.0.1.1") == 0)
        {
            host_ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[i]);
            break;
        }
        i++;
    }

    cout << "Host name: " << host->h_name << "\n"
         << "IP: " << host_ip << "\n"
         << "Port " << port << endl;

    int server_listening_socket = socket(AF_INET, SOCK_DGRAM, 0); // 创建套接字
    if (server_listening_socket < 0)
    {
        cerr << "socket create error" << endl;
        return -1;
    }
    int opt = 1;
    if (setsockopt(server_listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "setsockopt failed" << endl;
         return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // 清空struck
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // struct sockaddr_in
    // {
    //     short sin_family;        // 地址类型 AF_INET
    //     unsigned short sin_port; // 端口号 用ntohs转换成Char*
    //     struct in_addr sin_addr; // IP地址 用inet_ntoa转换成Char*
    //     char sin_zero[8];        // 填充
    // };

    int bin = bind(server_listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // 绑定套接字到地址和端口
    //(struct sockaddr *)&server_addr 将server_addr转换成更加通用的sockaddr类型。bind只接受这个类型
    if (bin < 0){
        cerr << "bind faild" << endl;
    }
    while (true)
    {
        struct sockaddr_in client_addr;
        char buffer[1024];
        socklen_t client_len = sizeof(client_addr);
        memset(buffer, 0, sizeof(buffer));
        int incoming_data = recvfrom(server_listening_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_len);
        if (incoming_data <= 0)
        {
            cerr << "no data recive" << endl;
        }
        buffer[incoming_data] = '\0';
        message_num++;
        char *heap_buffer = new char[incoming_data + 1];
        memcpy(heap_buffer, buffer, incoming_data);
        heap_buffer[incoming_data] = '\0'; 
        std::thread t(UDP_server_thread, heap_buffer);
        t.detach();
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
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

    port = atoi(argv[2]);
    if (port < 0 || port > 65535)
    {
        cerr << "bad port number\n";
        return 1;
    }
    
    if (mode == "-t"){
        TCP_server();
    }
    if (mode == "-u"){
        UDP_server();
    }
}