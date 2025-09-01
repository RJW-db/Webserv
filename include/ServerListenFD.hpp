#ifndef SERVERLISTENFD_HPP
#define SERVERLISTENFD_HPP

class ServerListenFD
{
    public:
        ServerListenFD(const char *port, const char *hostname);
        ~ServerListenFD();

        int	getFD() const;

        void createListenerSocket();
        struct addrinfo *getServerAddrinfo(void);
        void bindToSocket(struct addrinfo *server);

    private:
        int         _listener;
        const char *_port;
        const char *_hostName;
};
#endif
