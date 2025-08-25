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

#include "Constants.hpp"

class HttpRequest
{
    public:
        // --- Parsing ---
        static bool parseHttpHeader(Client &client, const char *buff, size_t receivedBytes);
        static bool parseHttpBody(Client &client, const char* buff, size_t receivedBytes);
        static bool processHttpBody(Client &client);
        static inline bool appendToBody(Client &client, const char *buff, size_t receivedBytes)
        {
            client._body.append(buff, receivedBytes);
            return (true);
        }
        static void validateHEAD(Client &client);
        static void	getBodyInfo(Client &client, const string buff);
        static void getContentLength(Client &client);
        static void getContentType(Client &client);

        // --- Request Processing ---
        static void	processRequest(Client &client);
        static bool checkAndRedirect(Client &client);
        static void redirectRequest(Client &client);
        static bool checkAndRunCgi(Client &client);
        static bool handleCgi(Client &client, string &body);

        // --- HTTP Methods ---
        static void processHead(Client &client);
        static void processGet(Client &client);
        static void processPost(Client &client);
        static void processDelete(Client &client);
        
        // --- File Operations ---
        static void	GET(Client &client);
        static void	POST(Client &client);
        static void SendAutoIndex(Client &client);
        static void appendUuidToFilename(Client &client, string &filename);

        // --- Response Generation ---
        static string getMimeType(string &path);
        static string HttpResponse(Client &client, uint16_t code, string path, size_t fileSize);
        static string createResponseCgi(Client &client, string &input);
};