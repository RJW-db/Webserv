#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define RESERVED_FDS 5         // stdin, stdout, stderr, server, epoll
#define INCOMING_FD_LIMIT 1024 // Makefile, shell(ulimit -n)
// TODO if FD minimum = 5, immediate turning off webserv with error.
// 4th is listener FD, 5th FD is client.
#define FD_LIMIT 1024 - RESERVED_FDS

#define CLIENT_BUFFER_SIZE 4096
#define PORT "8080"

# define _XOPEN_SOURCE 700  // VSC related, make signal and struct visisible
#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <string_view>


using namespace std;
// #include <FileDescriptor.hpp>
class FileDescriptor;


typedef struct _tmp
{
	string hostname;
	const char *port;
}	tmp_t;

struct ClientRequestState
{
    bool headerParsed = false;
    string header;
    string body;
    string method;
    size_t contentLength = 0;
};

class RunServers;
using ServerList = vector<unique_ptr<RunServers>>;

class RunServers
{
    public:
        RunServers(tmp_t *serverConf);
        ~RunServers();

        static int epollInit(ServerList &servers);
        // int run(FileDescriptor& fds);
        // static int runServers(vector<Server>& servers, FileDescriptor& fds);
        static int runServers(ServerList& servers, FileDescriptor& fds);
        static void handleEvents(ServerList& servers, FileDescriptor& fds, size_t eventCount);
        static void acceptConnection(const unique_ptr<RunServers> &server, FileDescriptor& fds);
        static void processClientRequest(const unique_ptr<RunServers> &server, FileDescriptor& fds, int clientFD);
        
        static int make_socket_non_blocking(int sfd);

        static bool directoryCheck(string &path);

        static void cleanupClient(int clientFD, FileDescriptor &fds);

        class ClientException : public std::exception
		{
            private:
                std::string _message;
			public:
                explicit ClientException(const std::string &message) : _message(message) {}
                virtual const char* what() const throw() {
                    return _message.c_str();
                }
		};

    private:
        string _serverName;
        int _listener; // moet weg

        static int _epfd;
        static array<struct epoll_event, FD_LIMIT> _events;

        static unordered_map<int, string> _fdBuffers;
        static unordered_map<int, ClientRequestState> _clientStates;
};


class ServerListenFD
{
    public:
        ServerListenFD(const char *port, const char *hostname);
        ~ServerListenFD();

        int	getFD() const;


        int create_listener_socket();
        struct addrinfo *get_server_addrinfo(void);
        int bind_to_socket(struct addrinfo *server);
    private:
        int         _listener;
        const char *_port;
        const char *_hostName;
};

void	poll_usages(void);
void	epoll_usage(void);
int		getaddrinfo_usage(void);
int		server(void);

void parseHttpRequest(string &request);
std::string escape_special_chars(const std::string& input);

void httpRequestLogger(string str);

string  extractMethod(const string &header);
string  extractHeader(const string &header, const string &key);
void    sendErrorResponse(int clientFD, const std::string &message);
void    handleRequest(int clientFD, string &method, string &header, string &body);
#endif
