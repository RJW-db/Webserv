#include <RunServer.hpp>
#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

#include <sys/epoll.h>

HandleWriteToCgiTransfer::HandleWriteToCgiTransfer(Client &client, string &body, int fdWriteToCgi)
: HandleTransfer(client, fdWriteToCgi), _bytesWrittenTotal(0)
{
    _fileBuffer = body;
    // _isCgi = writeToCgi;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandleReadFromCgiTransfer::HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi)
: HandleTransfer(client, fdReadfromCgi)
{
    // _isCgi = writeToCgi;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
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
    // std::cout << "double check "<< _fileBuffer.size() << std::endl; //testcout
    /**
     * you can expand the pipe buffer, normally is 65536, using fcntl F_SETPIPE_SZ.
     * check the maximum size: cat /proc/sys/fs/pipe-max-size
     */
    ssize_t sent = write(_fd, _fileBuffer.data() + _bytesWrittenTotal, _fileBuffer.size() - _bytesWrittenTotal);
    if (sent == -1)
    {
        throw ErrorCodeClientException(_client, 500, "Writing to CGI failed");
    }
    else if (sent > 0)
    {
        std::cout << "sent " << sent << std::endl; //testcout
        _bytesWrittenTotal += static_cast<size_t>(sent);
        if (_fileBuffer.size() == _bytesWrittenTotal)
        {
            RunServers::cleanupFD(_fd);
            // _isCgi = readFromCgi;
            std::cout << "finished sending" << std::endl; //testcout
            // FileDescriptor::closeFD(_fd);

            return true;
        }
    }
    return false;
}

bool HandleReadFromCgiTransfer::readFromCgiTransfer()
{
    std::vector<char> buff(500 * 1024);
    ssize_t rd = read(_fd, buff.data(), buff.size());
    write(1, "IT WORKED\n", 10);
    ssize_t sent = send(_client._fd, buff.data(), rd, 0);
    std::cout << "sent " << sent << std::endl; //testcout
    write(1, buff.data(), rd);
    return true;
}
