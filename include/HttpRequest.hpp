#include <iostream>
#include <FileDescriptor.hpp>
#include <Server.hpp>

#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>

#include <unordered_set>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>

#include <fstream>
#include <sstream>
#include <utils.hpp>
#include <sys/stat.h>
// #include <utils.hpp>
#include <Client.hpp>

using namespace std;

enum ContentType
{
    UNSUPPORTED,
    FORM_URLENCODED,
    JSON,
    TEXT,
    MULTIPART
};

class HttpRequest
{
    public:
        // HttpRequest(unique_ptr<Server> &server, int clientFD, string &method, string &header, string &body);
        // HttpRequest(unique_ptr<Server> &usedServer, Location &loc, int clientFD, Client &state);
        static bool parseHttpHeader(Client &client, const char *buff, size_t receivedBytes);
        static bool parseHttpBody(Client &client, const char* buff, size_t receivedBytes);
        static bool processHttpBody(Client &client);
        static void getInfoPost(Client &client, string &content, size_t &totalWriteSize);

        static inline bool appendToBody(Client &client, const char *buff, size_t receivedBytes)
        {
            client._body.append(buff, receivedBytes);
            return (true);
        }
        static inline bool findDelimiter(Client &client, size_t delimiter, size_t receivedBytes) {
            if (delimiter == std::string::npos) {
                if (receivedBytes == RunServers::getClientBufferSize())
                    return false;
                // throw runtime_error("for debugging purposes, remove this line, and kick client through keep-alive check");
            }
            return true;
        }

        static void validateHEAD(Client &client);
        static void	handleRequest(Client &client);

        static void	parseHeaders(Client &client);
        static void	getBodyInfo(Client &client);


        static void	POST(Client &client);

        static void	GET(Client &client);
        static void    findIndexFile(Client &client, struct stat &status);
        static void    locateRequestedFile(Client &client);

        static string getMimeType(string &path);
        static string HttpResponse(Client &client, uint16_t code, string path, size_t fileSize);

        static void getContentLength(Client &client);

        static void decodeSafeFilenameChars(Client &client);
        static ContentType getContentType(Client &client);

        static void handleChunks(Client &client);

        // Chunked request
        static void validateChunkSizeLine(const string &input);
        static uint64_t parseChunkSize(const string &input);
        static void ParseChunkStr(const string &input, uint64_t chunkTargetSize);

};