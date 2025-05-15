#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define RESERVED_FDS 5         // stdin, stdout, stderr, server, epoll
#define INCOMING_FD_LIMIT 1024 // Makefile, shell(ulimit -n)
// TODO if FD minimum = 5, immediate turning off webserv with error.
// 4th is listener FD, 5th FD is client.
#define FD_LIMIT 1024 - RESERVED_FDS

#define PORT "8080"

#include <iostream>
#include <memory>
#include <vector>
#include <array>

// #include <FileDescriptor.hpp>
class FileDescriptor;


typedef struct _tmp
{
	std::string hostname;
	const char *port;
}	tmp_t;

class Server;
using ServerList = std::vector<std::unique_ptr<Server>>;


class Server
{
    public:
        Server(tmp_t *serverConf);
        ~Server();

        static int epollInit(ServerList &servers);
        // int run(FileDescriptor& fds);
        static int make_socket_non_blocking(int sfd);
        // static int runServers(std::vector<Server>& servers, FileDescriptor& fds);
        static int runServers(ServerList& servers, FileDescriptor& fds);
        static void handleEvents(ServerList& servers, FileDescriptor& fds, int eventCount);
        // static void acceptConnection(const std::unique_ptr<Server>& server);
    private:
        std::string _serverName;
        int _listener;

        static bool _isRunning;
        static int _epfd;
        static std::array<struct epoll_event, FD_LIMIT> _events;
};


class ServerListenFD
{
    public:
        ServerListenFD(const char *port);
        ~ServerListenFD();

        int	getFD() const;


        int create_listener_socket();
        struct addrinfo *get_server_addrinfo(void);
        int bind_to_socket(struct addrinfo *server);

    private:
        int         _listener;
        const char *_port;
};

void	poll_usages(void);
void	epoll_usage(void);
int		getaddrinfo_usage(void);
int		server(void);
#endif
