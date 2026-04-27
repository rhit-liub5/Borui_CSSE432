#include "common.hpp"
#include "file_manager.hpp"

#include <algorithm>
#include <future>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std;

struct PeerSource
{
    string peer_id;
    string ip;
    int port = 0;
    set<int> pieces;
};

struct QueryResult
{
    string filename;
    long long filesize = 0;
    int piece_size = 0;
    int piece_count = 0;
    vector<PeerSource> peers;
};

static vector<string> tracker_request(const string &host, int port, const string &request)
{
    int sockfd = connect_to_host(host, port);
    if (sockfd < 0)
    {
        throw runtime_error("cannot connect to tracker");
    }
    send_line(sockfd, request);

    vector<string> lines;
    string line;
    while (recv_line(sockfd, line))
    {
        if (line == "END")
        {
            break;
        }
        lines.push_back(line);
    }
    close(sockfd);
    return lines;
}

static void print_lines(const vector<string> &lines)
{
    for (const string &line : lines)
    {
        cout << line << endl;
    }
}

static QueryResult parse_query(const vector<string> &lines)
{
    if (lines.empty() || lines[0].rfind("NOT_FOUND", 0) == 0)
    {
        throw runtime_error("file not found by tracker");
    }
    vector<string> header = split_words(lines[0]);
    if (header.size() != 5 || header[0] != "PEERS")
    {
        throw runtime_error("bad tracker response");
    }

    QueryResult result;
    result.filename = header[1];
    result.filesize = stoll(header[2]);
    result.piece_size = stoi(header[3]);
    result.piece_count = stoi(header[4]);

    for (size_t i = 1; i < lines.size(); ++i)
    {
        vector<string> parts = split_words(lines[i]);
        if (parts.size() != 4)
        {
            continue;
        }
        PeerSource source;
        source.peer_id = parts[0];
        source.ip = parts[1];
        source.port = stoi(parts[2]);
        for (int piece : split_ints(parts[3], ','))
        {
            source.pieces.insert(piece);
        }
        result.peers.push_back(source);
    }
    return result;
}

static vector<char> request_file_from_peer(const PeerSource &source, const string &filename)
{
    int sockfd = connect_to_host(source.ip, source.port);
    if (sockfd < 0)
    {
        throw runtime_error("cannot connect to peer " + source.peer_id);
    }
    send_line(sockfd, "GET_FILE " + filename);
    string line;
    if (!recv_line(sockfd, line))
    {
        close(sockfd);
        throw runtime_error("peer disconnected");
    }
    vector<string> header = split_words(line);
    if (header.size() >= 1 && header[0] == "ERROR")
    {
        close(sockfd);
        throw runtime_error(line);
    }
    if (header.size() != 2 || header[0] != "OK")
    {
        close(sockfd);
        throw runtime_error("bad peer response");
    }
    vector<char> data((size_t)stoll(header[1]));
    if (!recv_all(sockfd, data.data(), data.size()))
    {
        close(sockfd);
        throw runtime_error("incomplete file payload");
    }
    close(sockfd);
    return data;
}

static vector<char> request_piece_from_peer(const PeerSource &source, const string &filename, int piece_id)
{
    int sockfd = connect_to_host(source.ip, source.port);
    if (sockfd < 0)
    {
        throw runtime_error("cannot connect to peer " + source.peer_id);
    }
    send_line(sockfd, "GET_PIECE " + filename + " " + to_string(piece_id));
    string line;
    if (!recv_line(sockfd, line))
    {
        close(sockfd);
        throw runtime_error("peer disconnected");
    }
    vector<string> header = split_words(line);
    if (header.size() >= 1 && header[0] == "ERROR")
    {
        close(sockfd);
        throw runtime_error(line);
    }
    if (header.size() != 4 || header[0] != "PIECE" || stoi(header[2]) != piece_id)
    {
        close(sockfd);
        throw runtime_error("bad piece response");
    }
    vector<char> data((size_t)stoll(header[3]));
    if (!recv_all(sockfd, data.data(), data.size()))
    {
        close(sockfd);
        throw runtime_error("incomplete piece payload");
    }
    close(sockfd);
    return data;
}

static void handle_upload_client(int client_fd, FileManager &manager)
{
    string line;
    if (!recv_line(client_fd, line))
    {
        close(client_fd);
        return;
    }
    vector<string> parts = split_words(line);
    try
    {
        if (parts.size() == 2 && parts[0] == "GET_FILE")
        {
            vector<char> data = manager.read_file(parts[1]);
            send_line(client_fd, "OK " + to_string(data.size()));
            send_all(client_fd, data.data(), data.size());
        }
        else if (parts.size() == 3 && parts[0] == "GET_PIECE")
        {
            int piece_id = stoi(parts[2]);
            vector<char> data = manager.read_piece(parts[1], piece_id);
            send_line(client_fd, "PIECE " + parts[1] + " " + to_string(piece_id) + " " +
                                     to_string(data.size()));
            send_all(client_fd, data.data(), data.size());
        }
        else
        {
            send_line(client_fd, "ERROR unsupported command");
        }
    }
    catch (const exception &exc)
    {
        send_line(client_fd, string("ERROR ") + exc.what());
    }
    close(client_fd);
}

static void run_upload_server(const string &host, int port, FileManager &manager)
{
    int listen_fd = create_listen_socket(host, port);
    if (listen_fd < 0)
    {
        cerr << "failed to listen on " << host << ":" << port << endl;
        return;
    }
    cout << "[peer] serving on " << host << ":" << port << endl;
    while (true)
    {
        int client_fd = ::accept(listen_fd, nullptr, nullptr);
        if (client_fd >= 0)
        {
            thread(handle_upload_client, client_fd, ref(manager)).detach();
        }
    }
}

static const PeerSource *source_for_piece(const QueryResult &result, const string &self_id, int piece_id)
{
    for (const PeerSource &source : result.peers)
    {
        if (source.peer_id != self_id && source.pieces.count(piece_id))
        {
            return &source;
        }
    }
    for (const PeerSource &source : result.peers)
    {
        if (source.pieces.count(piece_id))
        {
            return &source;
        }
    }
    return nullptr;
}

static void print_help()
{
    cout << "commands:\n"
         << "  add <path> [filename]\n"
         << "  query <filename>\n"
         << "  list\n"
         << "  files\n"
         << "  download-file <filename>\n"
         << "  download-pieces <filename>\n"
         << "  have <filename>\n"
         << "  help\n"
         << "  quit\n";
}

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        cerr << "usage: ./peer <peer_id> <listen_port> <tracker_host> <tracker_port> <listen_host>\n";
        cerr << "example: ./peer A 9101 127.0.0.1 9000 127.0.0.1\n";
        return 1;
    }

    string peer_id = argv[1];
    int listen_port = stoi(argv[2]);
    string tracker_host = argv[3];
    int tracker_port = stoi(argv[4]);
    string listen_host = argv[5];

    FileManager manager("peer_data/" + peer_id + "/shared",
                        "peer_data/" + peer_id + "/downloads", 256 * 1024);

    thread(run_upload_server, listen_host, listen_port, ref(manager)).detach();
    print_lines(tracker_request(tracker_host, tracker_port,
                                "REGISTER " + peer_id + " " + listen_host + " " +
                                    to_string(listen_port)));
    print_help();

    string raw;
    while (cout << peer_id << "> " && getline(cin, raw))
    {
        vector<string> parts = split_words(raw);
        if (parts.empty())
        {
            continue;
        }
        string cmd = parts[0];
        try
        {
            if (cmd == "quit" || cmd == "exit")
            {
                break;
            }
            else if (cmd == "help")
            {
                print_help();
            }
            else if (cmd == "add" && (parts.size() == 2 || parts.size() == 3))
            {
                LocalFile info = manager.import_file(parts[1], parts.size() == 3 ? parts[2] : "");
                print_lines(tracker_request(tracker_host, tracker_port,
                                            "ADD_FILE " + peer_id + " " + info.filename + " " +
                                                to_string(info.filesize) + " " +
                                                to_string(info.piece_size) + " " +
                                                to_string(info.piece_count)));
            }
            else if (cmd == "query" && parts.size() == 2)
            {
                print_lines(tracker_request(tracker_host, tracker_port, "QUERY " + parts[1]));
            }
            else if (cmd == "list")
            {
                print_lines(tracker_request(tracker_host, tracker_port, "LIST"));
            }
            else if (cmd == "files")
            {
                for (const LocalFile &info : manager.list_complete_files())
                {
                    cout << info.filename << " " << info.filesize << " bytes "
                         << info.piece_count << " pieces" << endl;
                }
            }
            else if (cmd == "download-file" && parts.size() == 2)
            {
                QueryResult result = parse_query(tracker_request(tracker_host, tracker_port,
                                                                 "QUERY " + parts[1]));
                if (result.peers.empty())
                {
                    throw runtime_error("no source peers");
                }
                const PeerSource &source = result.peers[0];
                vector<char> data = request_file_from_peer(source, parts[1]);
                string out_path = manager.get_download_dir() + "/" + parts[1];
                FILE *fp = fopen(out_path.c_str(), "wb");
                fwrite(data.data(), 1, data.size(), fp);
                fclose(fp);
                LocalFile info = manager.local_file(parts[1]);
                print_lines(tracker_request(tracker_host, tracker_port,
                                            "ADD_FILE " + peer_id + " " + info.filename + " " +
                                                to_string(info.filesize) + " " +
                                                to_string(info.piece_size) + " " +
                                                to_string(info.piece_count)));
                cout << "downloaded " << parts[1] << " from " << source.peer_id << endl;
            }
            else if (cmd == "download-pieces" && parts.size() == 2)
            {
                QueryResult result = parse_query(tracker_request(tracker_host, tracker_port,
                                                                 "QUERY " + parts[1]));
                vector<int> have = manager.complete_piece_ids(parts[1], result.piece_count);
                set<int> have_set(have.begin(), have.end());

                vector<future<pair<int, vector<char>>>> jobs;
                for (int piece_id = 0; piece_id < result.piece_count; ++piece_id)
                {
                    if (have_set.count(piece_id))
                    {
                        continue;
                    }
                    const PeerSource *source = source_for_piece(result, peer_id, piece_id);
                    if (source == nullptr)
                    {
                        cout << "no source for piece " << piece_id << endl;
                        continue;
                    }
                    PeerSource copy = *source;
                    jobs.push_back(async(launch::async, [copy, filename = parts[1], piece_id]() {
                        return make_pair(piece_id, request_piece_from_peer(copy, filename, piece_id));
                    }));
                }

                for (auto &job : jobs)
                {
                    auto result_piece = job.get();
                    manager.write_piece(parts[1], result_piece.first, result_piece.second);
                    print_lines(tracker_request(tracker_host, tracker_port,
                                                "HAVE " + peer_id + " " + parts[1] + " " +
                                                    to_string(result_piece.first)));
                    cout << "piece " << result_piece.first << " downloaded" << endl;
                }

                have = manager.complete_piece_ids(parts[1], result.piece_count);
                if ((int)have.size() == result.piece_count)
                {
                    string out_path = manager.assemble_file(parts[1], result.piece_count);
                    LocalFile info = manager.local_file(parts[1]);
                    print_lines(tracker_request(tracker_host, tracker_port,
                                                "ADD_FILE " + peer_id + " " + info.filename + " " +
                                                    to_string(info.filesize) + " " +
                                                    to_string(info.piece_size) + " " +
                                                    to_string(info.piece_count)));
                    cout << "assembled " << out_path << endl;
                }
            }
            else if (cmd == "have" && parts.size() == 2)
            {
                LocalFile info = manager.local_file(parts[1]);
                for (int i = 0; i < info.piece_count; ++i)
                {
                    print_lines(tracker_request(tracker_host, tracker_port,
                                                "HAVE " + peer_id + " " + parts[1] + " " +
                                                    to_string(i)));
                }
            }
            else
            {
                cout << "ERROR unknown command; type help" << endl;
            }
        }
        catch (const exception &exc)
        {
            cout << "ERROR " << exc.what() << endl;
        }
    }
}
