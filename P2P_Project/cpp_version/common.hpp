#ifndef P2P_COMMON_HPP
#define P2P_COMMON_HPP

#include <cstddef>
#include <string>
#include <vector>

bool send_all(int sockfd, const char *data, std::size_t len);
bool recv_all(int sockfd, char *data, std::size_t len);
bool send_line(int sockfd, const std::string &line);
bool recv_line(int sockfd, std::string &line);
int connect_to_host(const std::string &host, int port);
int create_listen_socket(const std::string &host, int port);
std::vector<std::string> split_words(const std::string &line);
std::string join_ints(const std::vector<int> &values, char sep);
std::vector<int> split_ints(const std::string &text, char sep);
std::string basename_of(const std::string &path);

#endif
