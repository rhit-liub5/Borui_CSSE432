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
#include <fstream>
#include <sstream>
#include <sys/stat.h>

using namespace std;
int port;

bool starts_with(const string &s, const string &prefix)
{
    return s.rfind(prefix, 0) == 0;
}

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

bool recv_all(int sockfd, char *buffer, int total_len)
{
    int received_total = 0;

    while (received_total < total_len)
    {
        int received_now = recv(sockfd, buffer + received_total, total_len - received_total, 0);
        if (received_now <= 0)
        {
            return false;
        }
        received_total += received_now;
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
        int n = recv(sockfd, &ch, 1, 0);
        if (n <= 0)
        {
            return false;
        }

        if (ch == '\n')
        {
            break;
        }

        line += ch;
    }

    return true;
}

bool file_exists(const string &path)
{
    struct stat st;
    // struct stat 是一个极其核心的结构体。它的主要作用是存储文件或目录的元数据（Metadata），即文件的各种属性信息，而不包含文件实际存储的数据内容。
    return stat(path.c_str(), &st) == 0;
}
// 检查文件或目录是否存在

long get_file_size(const string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return -1;
    }
    return st.st_size;
}
// 获取文件大小（字节数）

string get_basename(const string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}
// 提取路径中的纯文件名

int handle_iwant(int comm_socket, const string &filename)
{
    string full_path = "store/" + filename;

    if (!file_exists(full_path))
    {
        send_line(comm_socket, "ERROR Cannot find that file");
        return -1;
    }

    long filesize = get_file_size(full_path);
    string base = get_basename(filename);

    string header = "FILE " + base + " " + to_string(filesize);
    if (!send_line(comm_socket, header))
    {
        cerr << "failed to send FILE header" << endl;
        return -1;
    }

    string client_reply;
    if (!recv_line(comm_socket, client_reply))
    {
        cerr << "client disconnected before READY" << endl;
        return -1;
    }

    if (client_reply != "READY")
    {
        cerr << "expected READY, got: " << client_reply << endl;
        return -1;
    }

    ifstream infile(full_path, ios::binary);
    // 用于读取文件,以二进制模式打开指定路径的文件，并准备从中读取数据
    if (!infile)
    {
        send_line(comm_socket, "ERROR Cannot open file");
        return -1;
    }

    char buffer[4096];
    while (infile)
    {
        infile.read(buffer, sizeof(buffer));
        streamsize bytes_read = infile.gcount();

        if (bytes_read > 0)
        {
            if (!send_all(comm_socket, buffer, (int)bytes_read))
            {
                cerr << "failed sending file data" << endl;
                return -1;
            }
        }
    }
    // 假设文件总大小是 5000 字节，buffer 是 4096 字节：

    // 第一次循环：infile 状态正常(true)。read() 读了 4096 字节。文件剩 904 字节。

    // 第二次循环开始：infile 依然正常(true)。

    // 第二次调用
    // read()：试图读 4096 字节，但文件只剩 904 字节了。read() 会把这最后 904 字节装进 buffer。关键来了： 因为它没能读够你要求的 4096 字节就撞到了文件尾部，此时 infile 内部会自动设置 eofbit 和 failbit。

    // 接着往下走，gcount() 依然能正确返回 904。这 904 字节的数据被成功处理。

    // 第三次循环开始：再次判断 while (infile)。因为上一步中 failbit 被设置了，所以隐式转换结果变为 false。循环立刻终止！
         
    cout << "Sent file: " << full_path << " (" << filesize << " bytes)" << endl;
    return 0;
}

int handle_utake(int comm_socket, const string &filename, long filesize)
{
    string save_dir = "received_file";
    string output_path = save_dir + "/" + get_basename(filename);
    cout << output_path <<endl;
    ofstream outfile(output_path, ios::binary);
    if (!outfile)
    {
        send_line(comm_socket, "ERROR Cannot open output file");
        return -1;
    }

    if (!send_line(comm_socket, "READY"))
    {
        cerr << "failed to send READY" << endl;
        return -1;
    }

    cout << "receiving file from client: " << filename
         << " (" << filesize << " bytes)" << endl;

    long remaining = filesize;
    char buffer[4096];

    while (remaining > 0)
    {
        int chunk = (remaining > (long)sizeof(buffer)) ? sizeof(buffer) : (int)remaining;

        if (!recv_all(comm_socket, buffer, chunk))
        {
            cerr << "failed while receiving uploaded file data" << endl;
            outfile.close();
            return -1;
        }

        outfile.write(buffer, chunk);
        remaining -= chunk;
    }

    outfile.close();

    cout << "received file saved to " << output_path << endl;

    if (!send_line(comm_socket, "SUCCESS"))
    {
        cerr << "failed to send SUCCESS" << endl;
        return -1;
    }

    return 0;
}

void TCP_server_thread(int comm_socket, int client_num)
{
    char buffer[1024];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int incoming_data = recv(comm_socket, buffer, sizeof(buffer) - 1, 0);

        if (incoming_data <= 0)
        {
            cerr << "client disconnected" << endl;
            break;
        }

        buffer[incoming_data] = '\0';
        string request(buffer);
        string response;

        cout << "Received from client " << client_num << ": " << request << endl;

        if (starts_with(request, "IWANT "))
        {
            string filename = request.substr(6);
            filename.pop_back(); // 去掉末尾的换行符
            if (filename.empty())
            {
                send_line(comm_socket, "ERROR That just ain't right!");
                continue;
            }

            handle_iwant(comm_socket, filename);
        }
        else if (starts_with(request, "UTAKE "))
        {
            stringstream ss(request);
            string cmd, filename;
            long filesize;

            ss >> cmd >> filename >> filesize;

            if (cmd != "UTAKE" || filename.empty() || filesize < 0)
            {
                send_line(comm_socket, "ERROR That just ain't right!");
                continue;
            }

            handle_utake(comm_socket, filename, filesize);
            continue;
        }
        else
        {
            response = "ERROR That just ain't right!";
        }

        // if (!send_all(comm_socket, response.c_str(), response.length()))
        // {
        //     cerr << "send failed" << endl;
        //     break;
        // }
    }

    close(comm_socket);
}

int TCP_server()
{
    char host_name[256];
    if (gethostname(host_name, 256) == -1)
    {
        cerr << "Error getting hostname" << endl;
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
             <<
            // "Aliases: " << host->h_aliases[i] << "\n" <<
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
    for (int i = 0; host->h_addr_list[i] != NULL; i++)
    {
        string ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[i]);
        if (ip != "127.0.0.1")
        {
            host_ip = ip;
            break;
        }
    }
    if (host_ip == "")
    {
        host_ip = "127.0.0.1";
    }

    cout << "Host name: " << host->h_name << "\n"
         << "IP: " << host_ip << "\n"
         << "Port " << port << endl;

    int server_listening_socket = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
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

    //(struct sockaddr *)&server_addr 将server_addr转换成更加通用的sockaddr类型。bind只接受这个类型
    if (::bind(server_listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "bind failed" << endl;
        return -1;
    }
    int lis = listen(server_listening_socket, 5); // 开始监听连接，参数为最大连接数
    if (lis < 0)
    {
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

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // socklen_t 是一种专门用来表示 socket 地址长度的类型。
        // 本质上它其实就是一个整数类型（类似 int），只是专门用于 socket API。
        int server_comm_socket = accept(server_listening_socket, (struct sockaddr *)&client_addr, &client_len);
        if (server_comm_socket < 0)
        {
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
        // TCP_server_thread(server_comm_socket, client_num);

        client_num++;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: ./server <port-number>" << endl;
        return 1;
    }

    port = atoi(argv[1]);
    if (port < 0 || port > 65535)
    {
        cerr << "bad port number" << endl;
        return 1;
    }

    return TCP_server();
}