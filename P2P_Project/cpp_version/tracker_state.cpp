#include "tracker_state.hpp"

#include "common.hpp"

#include <map>

using namespace std;

void TrackerState::register_peer(const string &peer_id, const string &ip, int port)
{
    lock_guard<mutex> lock(mtx);
    peers[peer_id] = PeerInfo{peer_id, ip, port};
}

void TrackerState::add_file(const string &peer_id, const string &filename,
                            long long filesize, int piece_size, int piece_count)
{
    lock_guard<mutex> lock(mtx);
    FileInfo &info = files[filename];
    info.filename = filename;
    info.filesize = filesize;
    info.piece_size = piece_size;
    info.piece_count = piece_count;

    for (int piece_id = 0; piece_id < piece_count; ++piece_id)
    {
        info.piece_owners[piece_id].insert(peer_id);
    }
}

bool TrackerState::have_piece(const string &peer_id, const string &filename, int piece_id,
                              string &error)
{
    lock_guard<mutex> lock(mtx);
    auto it = files.find(filename);
    if (it == files.end())
    {
        error = "unknown file";
        return false;
    }
    if (piece_id < 0 || piece_id >= it->second.piece_count)
    {
        error = "piece id out of range";
        return false;
    }
    it->second.piece_owners[piece_id].insert(peer_id);
    return true;
}

vector<string> TrackerState::query(const string &filename)
{
    lock_guard<mutex> lock(mtx);
    auto it = files.find(filename);
    if (it == files.end())
    {
        return {"NOT_FOUND " + filename};
    }

    const FileInfo &info = it->second;
    vector<string> lines;
    lines.push_back("PEERS " + info.filename + " " + to_string(info.filesize) + " " +
                    to_string(info.piece_size) + " " + to_string(info.piece_count));

    map<string, vector<int>> by_peer;
    for (const auto &entry : info.piece_owners)
    {
        int piece_id = entry.first;
        for (const string &peer_id : entry.second)
        {
            by_peer[peer_id].push_back(piece_id);
        }
    }

    for (const auto &entry : by_peer)
    {
        auto peer_it = peers.find(entry.first);
        if (peer_it == peers.end())
        {
            continue;
        }
        const PeerInfo &peer = peer_it->second;
        lines.push_back(peer.peer_id + " " + peer.ip + " " + to_string(peer.port) + " " +
                        join_ints(entry.second, ','));
    }
    return lines;
}

vector<string> TrackerState::list() const
{
    lock_guard<mutex> lock(mtx);
    vector<string> lines;
    lines.push_back("PEER_COUNT " + to_string(peers.size()));
    lines.push_back("FILE_COUNT " + to_string(files.size()));
    for (const auto &entry : files)
    {
        const FileInfo &info = entry.second;
        lines.push_back("FILE " + info.filename + " " + to_string(info.filesize) + " " +
                        to_string(info.piece_size) + " " + to_string(info.piece_count));
    }
    return lines;
}
