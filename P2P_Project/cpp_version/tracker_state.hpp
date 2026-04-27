#ifndef P2P_TRACKER_STATE_HPP
#define P2P_TRACKER_STATE_HPP

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

struct PeerInfo
{
    std::string peer_id;
    std::string ip;
    int port;
};

struct FileInfo
{
    std::string filename;
    long long filesize = 0;
    int piece_size = 0;
    int piece_count = 0;
    std::map<int, std::set<std::string>> piece_owners;
};

class TrackerState
{
public:
    void register_peer(const std::string &peer_id, const std::string &ip, int port);
    void add_file(const std::string &peer_id, const std::string &filename,
                  long long filesize, int piece_size, int piece_count);
    bool have_piece(const std::string &peer_id, const std::string &filename, int piece_id,
                    std::string &error);
    std::vector<std::string> query(const std::string &filename);
    std::vector<std::string> list() const;

private:
    mutable std::mutex mtx;
    std::map<std::string, PeerInfo> peers;
    std::map<std::string, FileInfo> files;
};

#endif
