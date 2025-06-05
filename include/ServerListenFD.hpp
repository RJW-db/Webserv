#ifndef SERVERLISTENFD_HPP
#define SERVERLISTENFD_HPP
#include <FileDescriptor.hpp>


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

#endif