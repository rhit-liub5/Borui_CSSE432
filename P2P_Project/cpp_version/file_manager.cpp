#include "file_manager.hpp"

#include "common.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace std;
namespace fs = std::filesystem;

static long long file_size_of(const string &path)
{
    return (long long)fs::file_size(path);
}

FileManager::FileManager(string shared, string downloads, int piece)
    : shared_dir(std::move(shared)), download_dir(std::move(downloads)),
      parts_dir(download_dir + "/.parts"), piece_size(piece)
{
    fs::create_directories(shared_dir);
    fs::create_directories(download_dir);
    fs::create_directories(parts_dir);
}

string FileManager::shared_path(const string &filename) const
{
    return shared_dir + "/" + filename;
}

string FileManager::download_path(const string &filename) const
{
    return download_dir + "/" + filename;
}

string FileManager::piece_path(const string &filename, int piece_id) const
{
    return parts_dir + "/" + filename + ".part" + to_string(piece_id);
}

LocalFile FileManager::import_file(const string &source_path, const string &filename)
{
    string name = filename.empty() ? basename_of(source_path) : filename;
    string target = shared_path(name);
    if (fs::absolute(source_path) != fs::absolute(target))
    {
        fs::copy_file(source_path, target, fs::copy_options::overwrite_existing);
    }
    return local_file(name);
}

LocalFile FileManager::local_file(const string &filename) const
{
    string path = shared_path(filename);
    if (!fs::exists(path))
    {
        path = download_path(filename);
    }
    if (!fs::exists(path))
    {
        throw runtime_error("file not found: " + filename);
    }
    long long size = file_size_of(path);
    int count = (int)((size + piece_size - 1) / piece_size);
    if (count < 1)
    {
        count = 1;
    }
    return LocalFile{filename, path, size, piece_size, count};
}

vector<LocalFile> FileManager::list_complete_files() const
{
    vector<LocalFile> files;
    for (const string &dir : {shared_dir, download_dir})
    {
        if (!fs::exists(dir))
        {
            continue;
        }
        for (const auto &entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                files.push_back(local_file(entry.path().filename().string()));
            }
        }
    }
    return files;
}

vector<char> FileManager::read_file(const string &filename) const
{
    LocalFile info = local_file(filename);
    vector<char> data((size_t)info.filesize);
    ifstream in(info.path, ios::binary);
    in.read(data.data(), (streamsize)data.size());
    return data;
}

vector<char> FileManager::read_piece(const string &filename, int piece_id) const
{
    string part_path = piece_path(filename, piece_id);
    if (fs::exists(part_path))
    {
        long long size = file_size_of(part_path);
        vector<char> data((size_t)size);
        ifstream in(part_path, ios::binary);
        in.read(data.data(), (streamsize)data.size());
        return data;
    }

    LocalFile info = local_file(filename);
    ifstream in(info.path, ios::binary);
    in.seekg((long long)piece_id * info.piece_size);
    vector<char> data((size_t)info.piece_size);
    in.read(data.data(), (streamsize)data.size());
    data.resize((size_t)in.gcount());
    return data;
}

void FileManager::write_piece(const string &filename, int piece_id, const vector<char> &data) const
{
    ofstream out(piece_path(filename, piece_id), ios::binary);
    out.write(data.data(), (streamsize)data.size());
}

vector<int> FileManager::complete_piece_ids(const string &filename, int piece_count) const
{
    try
    {
        LocalFile info = local_file(filename);
        vector<int> pieces;
        for (int i = 0; i < info.piece_count; ++i)
        {
            pieces.push_back(i);
        }
        return pieces;
    }
    catch (const exception &)
    {
        vector<int> pieces;
        for (int i = 0; i < piece_count; ++i)
        {
            if (fs::exists(piece_path(filename, i)))
            {
                pieces.push_back(i);
            }
        }
        return pieces;
    }
}

string FileManager::assemble_file(const string &filename, int piece_count) const
{
    string target = download_path(filename);
    ofstream out(target, ios::binary);
    for (int i = 0; i < piece_count; ++i)
    {
        string part = piece_path(filename, i);
        if (!fs::exists(part))
        {
            throw runtime_error("missing piece " + to_string(i));
        }
        ifstream in(part, ios::binary);
        vector<char> data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        out.write(data.data(), (streamsize)data.size());
    }
    return target;
}

string FileManager::get_download_dir() const
{
    return download_dir;
}
