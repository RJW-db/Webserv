#include <iostream>
#include <FileDescriptor.hpp>
#include <Server.hpp>

#include <RunServer.hpp>

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

        static void validateHEAD(Client &client);
        static void	handleRequest(Client &client);

        static void	parseHeaders(Client &client);
        static void	getBodyInfo(string &body);

        static void	POST(Client &client);

        static void	GET(Client &client);
        static void    locateRequestedFile(Client &client);

        static string getMimeType(string &path);
        static string HttpResponse(uint16_t code, string path, size_t fileSize);

        static void getContentLength(Client &client, const string_view content);


        static ContentType getContentType(const string_view ct);
		// static Location &setLocation();

        
    private:
        // Client &_client;

        // string_view _contentType;
        // string_view _bodyBoundary;
        
        // string_view _filename;
        // string_view _fileContent;
};