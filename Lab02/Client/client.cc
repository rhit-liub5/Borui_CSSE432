#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

using namespace std;
int port;
char *address;

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
    return stat(path.c_str(), &st) == 0;
}

long get_file_size(const string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return -1;
    }
    return st.st_size;
}

string get_basename(const string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

int handle_iwant(int client_socket, const string &filename)
{
    string save_dir;
    cout << "What directory would you like to save this file? ";
    getline(cin, save_dir);

    if (save_dir.empty())
    {
        save_dir = "received_files";
    }

    if (!send_line(client_socket, "IWANT " + filename))
    {
        cerr << "send failed" << endl;
        return -1;
    }

    string response;
    if (!recv_line(client_socket, response))
    {
        cerr << "server disconnected" << endl;
        return -1;
    }

    if (starts_with(response, "ERROR "))
    {
        cout << response << endl;
        return -1;
    }

    string tag, out_filename;
    long filesize;
    stringstream ss(response);
    ss >> tag >> out_filename >> filesize;

    if (tag != "FILE")
    {
        cout << "Bad response from server: " << response << endl;
        return -1;
    }

    string output_path = save_dir + "/" + out_filename;

    ofstream outfile(output_path, ios::binary);
    if (!outfile)
    {
        cout << "Failure: cannot open local output file" << endl;
        return -1;
    }

    if (!send_line(client_socket, "READY"))
    {
        cerr << "failed to send READY" << endl;
        return -1;
    }

    cout << "file transfer started..." << endl;

    long remaining = filesize;
    char buffer[4096];

    while (remaining > 0)
    {
        int chunk = (remaining > (long)sizeof(buffer)) ? sizeof(buffer) : (int)remaining;

        if (!recv_all(client_socket, buffer, chunk))
        {
            cerr << "failed while receiving file data" << endl;
            outfile.close();
            return -1;
        }

        outfile.write(buffer, chunk);
        remaining -= chunk;
    }

    outfile.close();

    cout << "file transfer of " << filesize
         << " bytes complete and placed in " << save_dir << endl;

    return 0;
}

int handle_utake(int client_socket, const string &filename)
{
    if (!file_exists(filename))
    {
        cout << "Failure: Cannot find that file" << endl;
        return -1;
    }

    long filesize = get_file_size(filename);
    if (filesize < 0)
    {
        cout << "Failure: Cannot get file size" << endl;
        return -1;
    }

    string base = get_basename(filename);
    string header = "UTAKE " + base + " " + to_string(filesize);

    if (!send_line(client_socket, header))
    {
        cerr << "failed to send UTAKE header" << endl;
        return -1;
    }

    string server_reply;
    if (!recv_line(client_socket, server_reply))
    {
        cerr << "server disconnected before READY" << endl;
        return -1;
    }

    if (server_reply != "READY")
    {
        cout << "Bad response from server: " << server_reply << endl;
        return -1;
    }

    ifstream infile(filename, ios::binary);
    if (!infile)
    {
        cout << "Failure: cannot open local file" << endl;
        return -1;
    }

    cout << "file transfer started..." << endl;

    char buffer[4096];
    while (infile)
    {
        infile.read(buffer, sizeof(buffer));
        streamsize bytes_read = infile.gcount();

        if (bytes_read > 0)
        {
            if (!send_all(client_socket, buffer, (int)bytes_read))
            {
                cerr << "failed while sending file data" << endl;
                return -1;
            }
        }
    }

    string final_reply;
    if (!recv_line(client_socket, final_reply))
    {
        cerr << "server disconnected after upload" << endl;
        return -1;
    }

    if (final_reply == "SUCCESS")
    {
        cout << "file transfer of " << filesize
             << " bytes to server complete" << endl;
        return 0;
    }

    cout << "Bad final response from server: " << final_reply << endl;
    return -1;
}

int TCP_client()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        cerr << "socket failed" << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (!resolve_host(address, server_addr))
    {
        cerr << "cannot resolve hostname or IP address" << endl;
        close(client_socket);
        return -1;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "connect failed" << endl;
        close(client_socket);
        return -1;
    }

    cout << "Connected to server." << endl;

    string message;
    while (true)
    {
        cout << "prompt> ";
        getline(cin, message);

        if (message == "exit")
        {
            break;
        }

        string to_send;

        if (starts_with(message, "iWant "))
        {
            string filename = message.substr(6);
            if (filename.empty())
            {
                cout << "That just ain't right!" << endl;
                continue;
            }
            handle_iwant(client_socket, filename);
        }
        else if (starts_with(message, "uTake "))
        {
            string filename = message.substr(6);
            if (filename.empty())
            {
                cout << "That just ain't right!" << endl;
                continue;
            }

            handle_utake(client_socket, filename);
        }
        else
        {
            cout << "That just ain't right!" << endl;
            continue;
        }

        // if (!send_all(client_socket, to_send.c_str(), to_send.length()))
        // {
        //     cerr << "send failed" << endl;
        //     close(client_socket);
        //     return -1;
        // }

        // char buffer[1024];
        // int data = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        // if (data <= 0)
        // {
        //     cout << "Server disconnected." << endl;
        //     break;
        // }

        // buffer[data] = '\0';
        // cout << buffer << endl;
    }

    close(client_socket);
    cout << "Shut down successful... goodbye" << endl;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: ./client <server-address> <port-number>" << endl;
        return 1;
    }

    address = argv[1];

    port = atoi(argv[2]);
    if (port < 0 || port > 65535)
    {
        cerr << "bad port number" << endl;
        return 1;
    }

    return TCP_client();
}