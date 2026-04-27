#ifndef P2P_FILE_MANAGER_HPP
#define P2P_FILE_MANAGER_HPP

#include <string>
#include <vector>

struct LocalFile
{
    std::string filename;
    std::string path;
    long long filesize = 0;
    int piece_size = 0;
    int piece_count = 0;
};

class FileManager
{
public:
    FileManager(std::string shared_dir, std::string download_dir, int piece_size);

    LocalFile import_file(const std::string &source_path, const std::string &filename);
    LocalFile local_file(const std::string &filename) const;
    std::vector<LocalFile> list_complete_files() const;
    std::vector<char> read_file(const std::string &filename) const;
    std::vector<char> read_piece(const std::string &filename, int piece_id) const;
    void write_piece(const std::string &filename, int piece_id, const std::vector<char> &data) const;
    std::vector<int> complete_piece_ids(const std::string &filename, int piece_count) const;
    std::string assemble_file(const std::string &filename, int piece_count) const;
    std::string get_download_dir() const;

private:
    std::string shared_dir;
    std::string download_dir;
    std::string parts_dir;
    int piece_size;

    std::string shared_path(const std::string &filename) const;
    std::string download_path(const std::string &filename) const;
    std::string piece_path(const std::string &filename, int piece_id) const;
};

#endif
