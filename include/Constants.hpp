#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

constexpr size_t DISCONNECT_DELAY_SECONDS = 500;
constexpr char CRLF[] = "\r\n";
constexpr size_t CRLF_LEN = sizeof(CRLF) - 1;
constexpr char CRLF2[] = "\r\n\r\n";
constexpr size_t CRLF2_LEN = sizeof(CRLF2) - 1;
constexpr size_t BOUNDARY_PREFIX_LEN = 2;  // For "--" boundary prefix
constexpr size_t PIPE_BUFFER_SIZE = 512 * 1024; // 512 KB
#endif
