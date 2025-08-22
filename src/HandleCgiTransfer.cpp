#include <RunServer.hpp>
#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

#include <sys/epoll.h>

#define CGI_DISCONNECT_TIME_SECONDS 30

HandleWriteToCgiTransfer::HandleWriteToCgiTransfer(Client &client, string &body, int fdWriteToCgi)
: HandleTransfer(client, fdWriteToCgi, HANDLE_WRITE_TO_CGI_TRANSFER), _bytesWrittenTotal(0)
{
    _fileBuffer = body;
    // _isCgi = writeToCgi;
    _handleType = HANDLE_WRITE_TO_CGI_TRANSFER;
    _cgiDisconnectTime = chrono::steady_clock::now() + chrono::seconds(CGI_DISCONNECT_TIME_SECONDS);
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandleReadFromCgiTransfer::HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi)
: HandleTransfer(client, fdReadfromCgi, HANDLE_READ_FROM_CGI_TRANSFER)
{
    // _isCgi = writeToCgi;
    _handleType = HANDLE_READ_FROM_CGI_TRANSFER;
    _cgiDisconnectTime = chrono::steady_clock::now() + chrono::seconds(CGI_DISCONNECT_TIME_SECONDS);
}



// HandleTransfer &HandleTransfer::operator=(const HandleTransfer& other)
// {
//     if (this != &other) {
        
//         // _client is a reference and cannot be assigned
//         _fd = other._fd;
//         _fileBuffer = other._fileBuffer;
//         // _fileSize = other._fileSize;
//         // _offset = other._offset;
//         // _bytesReadTotal = other._bytesReadTotal;
//         // _headerSize = other._headerSize;
// 		// _bytesWrittenTotal = other._bytesWrittenTotal;
//     }
//     return *this;
// }

bool HandleWriteToCgiTransfer::writeToCgiTransfer()
{
    ssize_t sent = write(_fd, _fileBuffer.data() + _bytesWrittenTotal, _fileBuffer.size() - _bytesWrittenTotal);
    if (sent == -1)
    {
        _client._cgiClosing = true;
        // return true;
        throw ErrorCodeClientException(_client, 500, "Writing to CGI failed");
    }
    else if (sent > 0)
    {
        _bytesWrittenTotal += static_cast<size_t>(sent);
        _client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
        if (_fileBuffer.size() == _bytesWrittenTotal)
        {
            FileDescriptor::cleanupFD(_fd);
            return true;
        }
    }
    return false;
}

bool HandleReadFromCgiTransfer::readFromCgiTransfer()
{
    std::vector<char> buff(1024 * 1024);
    ssize_t rd = read(_fd, buff.data(), buff.size());
    if (rd == -1)
    {
        FileDescriptor::cleanupFD(_fd);
        throw ErrorCodeClientException(_client, 500, "Reading from CGI failed: " + string(strerror(errno)));
    }
    _fileBuffer.append(buff.data(), rd);

    if (rd == 0 || (rd > 0 && buff[rd] == '\0'))
    {
        FileDescriptor::cleanupFD(_fd);
        // auto handle = make_unique<HandleToClientTransfer>(_client, _fileBuffer);
        // RunServers::insertHandleTransfer(move(handle));
        // RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
        _client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
        return true;
    }
    return false;
}
