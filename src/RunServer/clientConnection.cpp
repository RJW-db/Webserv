#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <netinet/in.h>   // sockaddr_in, ntohl, ntohs
#include <sys/socket.h>   // socket functions, accept
#include "RunServer.hpp"
#include "HttpRequest.hpp"
#include "Logger.hpp"
#include "ErrorCodeClientException.hpp"

#ifndef CLIENT_BUFFER_SIZE
# define CLIENT_BUFFER_SIZE 8192 // 8KB
#endif

namespace
{
    string NumIpToString(uint32_t num);
}

void RunServers::acceptConnection(const int listener)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        int infd = accept(listener, &in_addr, &in_len);
        if (infd == -1)
        {
            if (errno != EAGAIN)
                Logger::log(ERROR, "Server error", listener, "Accept failed", strerror(errno));
            break;
        }

        if (addFdToEpoll(infd) == false)
            break;

        _clients[infd] = std::make_unique<Client>(infd);
        setClientServerAddress(*_clients[infd], infd);

        Logger::log(INFO, *_clients[infd], 
            "Connected on: " + NumIpToString(ntohl(((sockaddr_in *)&in_addr)->sin_addr.s_addr)) + 
            ":" + std::to_string(ntohs(((sockaddr_in *)&in_addr)->sin_port)));

        _clients[infd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    }
}

bool RunServers::addFdToEpoll(int infd)
{
    struct epoll_event  current_event;
    current_event.data.fd = infd;
    current_event.events = EPOLLIN;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
    {
        Logger::log(ERROR, "Server error", infd, "epoll_ctl failed: ", strerror(errno));
        if (FileDescriptor::safeCloseFD(infd) == false)
            Logger::logExit(FATAL, "FileDescriptor error", infd, "safeCloseFD failed, FD not in vector");
        return false;
    }
    FileDescriptor::setFD(infd);
    return true;
}

void RunServers::setClientServerAddress(Client &client, int infd)
{
    sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr); 
    if (getsockname(infd, (struct sockaddr*)&serverAddr, &addrLen) != 0)
        throw ErrorCodeClientException(*_clients[infd], 500, "Failed to get server info");
    client._ipPort.first = NumIpToString(ntohl(serverAddr.sin_addr.s_addr));
    client._ipPort.second = to_string(ntohs(serverAddr.sin_port));
}

void RunServers::processClientRequest(Client &client)
{
    try
    {
        char   buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = receiveClientData(client, buff);
        static bool (*const handlers[4])(Client&, const char*, size_t) = {
            &HttpRequest::parseHttpHeader,                     // HEADER_AWAITING (0)
            &HttpRequest::appendToBody,                        // BODY_CHUNKED (1)
            &HttpRequest::parseHttpBody,                       // BODY_AWAITING (2)
            [](Client&, const char*, size_t) { return true; }  // REQUEST_READY (3)
        };
        if (handlers[client._headerParseState](client, buff, bytesReceived) == false)
            return ;
        // client._finishedProcessClientRequest = true;
        HttpRequest::processRequest(client);
    }
    catch(const exception& e)
    {
        throw ErrorCodeClientException(client, 500, "error occured in processclientRequest: " + string(e.what()));
    }

}

size_t RunServers::receiveClientData(Client &client, char *buff)
{
    // buff[CLIENT_BUFFER_SIZE] = '\0'; // kan alleen aan voor testen anders kan het voor post problemen geven
    client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    ssize_t bytesReceived = recv(client._fd, buff, CLIENT_BUFFER_SIZE, 0);
    // std::cout << "received: " << escapeSpecialChars(string(buff, bytesReceived), TERMINAL) << std::endl; //testcout
    // exit(0);
    if (bytesReceived > 0)
        return static_cast<size_t>(bytesReceived);
    if (bytesReceived < 0)
    {
        throw ErrorCodeClientException(client, 0, "Recv failed from client with error: " + string(strerror(errno)));
    }
    if (bytesReceived == 0)
    {
        throw ErrorCodeClientException(client, 0, "kicking out client after read of 0");
    }
    return (0); // You never get here
}

namespace
{
    string NumIpToString(uint32_t addr)
    {
        unsigned char bytes[4];
        bytes[0] = static_cast<unsigned char>((addr >> 24) & 0xFF);
        bytes[1] = static_cast<unsigned char>((addr >> 16) & 0xFF);
        bytes[2] = static_cast<unsigned char>((addr >> 8) & 0xFF);
        bytes[3] = static_cast<unsigned char>(addr & 0xFF);
    
        return to_string(bytes[0]) + "." +
               to_string(bytes[1]) + "." +
               to_string(bytes[2]) + "." +
               to_string(bytes[3]);
    }
}

