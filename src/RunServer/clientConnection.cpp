#include <netinet/in.h>   // sockaddr_in, ntohl, ntohs
#include <sys/socket.h>   // socket functions, accept
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"
#ifndef CLIENT_BUFFER_SIZE
# define CLIENT_BUFFER_SIZE 8192 // 8KB
#endif

namespace
{
    string NumIpToString(uint32_t num);
}

void RunServers::acceptConnection(const int listener)
{
    while (true) {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        int infd = accept(listener, &in_addr, &in_len);
        if (infd == -1) {
            if (errno != EAGAIN)
                Logger::log(ERROR, "Server error", listener, "Accept failed", strerror(errno));
            break;
        }

        FileDescriptor::setFD(infd);
        try {
            _clients[infd] = make_unique<Client>(infd); 
        }
        catch(const std::exception& e) {
            FileDescriptor::closeFD(infd);
            Logger::log(ERROR, "Server error", listener, "Client creation failed", strerror(errno));
            throw;
        }
        
        setClientServerAddress(*_clients[infd], infd);
        setEpollEventsClient(*_clients[infd], infd, EPOLL_CTL_ADD, EPOLLIN);

        Logger::log(INFO, *_clients[infd], 
            "Connected on: " + NumIpToString(ntohl((reinterpret_cast<sockaddr_in *>(&in_addr))->sin_addr.s_addr)) + 
            ":" + to_string(ntohs((reinterpret_cast<sockaddr_in *>(&in_addr))->sin_port)));

        _clients[infd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    }
}

void RunServers::setClientServerAddress(Client &client, int infd)
{
    sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr); 
    if (getsockname(infd, reinterpret_cast<sockaddr*>(&serverAddr), &addrLen) != 0)
        throw ErrorCodeClientException(*_clients[infd], 500, "Failed to get server info");
    client._ipPort.first = NumIpToString(ntohl(serverAddr.sin_addr.s_addr));
    client._ipPort.second = to_string(ntohs(serverAddr.sin_port));

}

void RunServers::processClientRequest(Client &client)
{
    try {
        char   buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = receiveClientData(client, buff);
        static bool (*const handlers[3])(Client&, const char*, size_t) = {
            &HttpRequest::parseHttpHeader,                     // HEADER_AWAITING (0)
            &HttpRequest::appendToBody,                        // BODY_CHUNKED (1)
            [](Client&, const char*, size_t) { return true; }  // REQUEST_READY (2)
        };
        if (handlers[client._headerParseState](client, buff, bytesReceived) == false)
            return ;
        HttpRequest::processRequest(client);
    }
    catch (const Logger::ErrorLogExit&) {
        throw;
    }
    catch (const exception& e) {
        throw ErrorCodeClientException(client, 500, "error occured in processclientRequest: " + string(e.what()));
    }
}

size_t RunServers::receiveClientData(Client &client, char *buff)
{
    client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    ssize_t bytesReceived = recv(client._fd, buff, CLIENT_BUFFER_SIZE, 0);
    // Logger::log(DEBUG, string(buff, static_cast<size_t>(bytesReceived)));
    if (bytesReceived > 0)
        return static_cast<size_t>(bytesReceived);
    if (bytesReceived < 0)
        throw ErrorCodeClientException(client, 500, "Recv failed from client with error: " + string(strerror(errno)));
    throw ErrorCodeClientException(client, 0, "kicking out client after read of 0");
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
