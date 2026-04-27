#include "common.hpp"
#include "tracker_state.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std;

static vector<string> handle_command(TrackerState &state, const string &line)
{
    vector<string> parts = split_words(line);
    if (parts.empty())
    {
        return {"ERROR empty command"};
    }

    try
    {
        const string &cmd = parts[0];
        if (cmd == "REGISTER" && parts.size() == 4)
        {
            state.register_peer(parts[1], parts[2], stoi(parts[3]));
            return {"OK REGISTERED " + parts[1]};
        }
        if (cmd == "ADD_FILE" && parts.size() == 6)
        {
            state.add_file(parts[1], parts[2], stoll(parts[3]), stoi(parts[4]), stoi(parts[5]));
            return {"OK FILE_ADDED " + parts[2]};
        }
        if (cmd == "HAVE" && parts.size() == 4)
        {
            string error;
            if (!state.have_piece(parts[1], parts[2], stoi(parts[3]), error))
            {
                return {"ERROR " + error};
            }
            return {"OK HAVE " + parts[2] + " " + parts[3]};
        }
        if (cmd == "QUERY" && parts.size() == 2)
        {
            return state.query(parts[1]);
        }
        if (cmd == "LIST" && parts.size() == 1)
        {
            return state.list();
        }
    }
    catch (const exception &exc)
    {
        return {string("ERROR ") + exc.what()};
    }

    return {"ERROR unsupported or malformed command"};
}

static void handle_client(int client_fd, TrackerState &state)
{
    string line;
    if (!recv_line(client_fd, line))
    {
        close(client_fd);
        return;
    }

    cout << "[tracker] " << line << endl;
    vector<string> response = handle_command(state, line);
    for (const string &item : response)
    {
        send_line(client_fd, item);
    }
    send_line(client_fd, "END");
    close(client_fd);
}

int main(int argc, char *argv[])
{
    string host = "127.0.0.1";
    int port = 9000;
    if (argc >= 2)
    {
        port = stoi(argv[1]);
    }
    if (argc >= 3)
    {
        host = argv[2];
    }

    int listen_fd = create_listen_socket(host, port);
    if (listen_fd < 0)
    {
        cerr << "failed to listen on " << host << ":" << port << endl;
        return 1;
    }

    TrackerState state;
    cout << "[tracker] listening on " << host << ":" << port << endl;
    while (true)
    {
        int client_fd = ::accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0)
        {
            continue;
        }
        thread(handle_client, client_fd, ref(state)).detach();
    }
}
