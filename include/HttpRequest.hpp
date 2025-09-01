#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP
#ifdef __linux__
#include <sys/socket.h>
#include <dirent.h>
# include <sys/epoll.h>
#endif
#include "Client.hpp"
using namespace std;
class HttpRequest
{
    public:
        // Parsing
        static bool parseHttpHeader(Client &client, const char *buff, size_t receivedBytes);
        static inline bool appendToBody(Client &client, const char *buff, size_t receivedBytes)
        {
            client._body.append(buff, receivedBytes);
            return (true);
        }
        static bool parseHttpBody(Client &client, const char* buff, size_t receivedBytes);
        static void validateHEAD(Client &client);
        static void	getBodyInfo(Client &client, const string buff);
        static void getContentLength(Client &client);
        static void getContentType(Client &client);

        // Request Processing
        static void	processRequest(Client &client);
        static bool checkAndRedirect(Client &client);
        static void redirectRequest(Client &client);
        static bool checkAndRunCgi(Client &client);
        static bool handleCgi(Client &client, string &body);

        // HTTP Methods
        static void processHead(Client &client);
        static void processGet(Client &client);
        static void processPost(Client &client);
        static void processDelete(Client &client);
        
        // File Operations
        static void	GET(Client &client);
        static void POST(Client &client);
        static void SendAutoIndex(Client &client);
        static void appendUuidToFilename(Client &client, string &filename);

        // Response Generation
        static string getMimeType(string &path);
        static string HttpResponse(Client &client, uint16_t code, string path, size_t fileSize);

        // Cgi response generation
        static string createResponseCgi(Client &client, string &input);
        static map<string_view, string_view> parseCgiHeaders(const string &input, map<string_view,string_view> &headerfields, size_t headerSize);
        static void validateCgiHeaders(Client &client, const map<string_view, string_view> &headerFields, bool hasBody);
        static string buildCgiResponse(Client &client, const map<string_view, string_view> &headerFields, 
                                const string &input, size_t headerSize, bool hasBody);
};
#endif
