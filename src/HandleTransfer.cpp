#include <RunServer.hpp>
#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

#include <sys/epoll.h>






// // CGI
// HandleTransfer::HandleTransfer(Client &client, string &body, int fdWriteToCgi, int fdReadfromCgi)
// : _client(client), _fd(-1), _fileBuffer(body), _bytesWrittenTotal(0), _fdWriteToCgi(fdWriteToCgi), _fdReadfromCgi(fdReadfromCgi)
// {
//     _isCgi = writeToCgi;
//     RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
// }

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






// bool HandleTransfer::handleCgiTransfer()
// {
//     /**
//      * you can expand the pipe buffer, normally is 65536, using fcntl F_SETPIPE_SZ.
//      * check the maximum size: cat /proc/sys/fs/pipe-max-size
//      */
//     ssize_t sent = write(_fdWriteToCgi, _fileBuffer.data() + _bytesWrittenTotal, _fileBuffer.size() - _bytesWrittenTotal);
//     if (sent == -1)
//     {
//         throw ErrorCodeClientException(_client, 500, "Writing to CGI failed");
//     }
//     else if (sent > 0)
//     {
//         std::cout << "sent " << sent << std::endl; //testcout
//         _bytesWrittenTotal += static_cast<size_t>(sent);
//         if (_fileBuffer.size() == _bytesWrittenTotal)
//         {
//             RunServers::cleanupFD(_fdWriteToCgi);
//             _isCgi = readFromCgi;
//             std::cout << "finished sending" << std::endl; //testcout
            
//             return true;
//         }
//     }
//     return false;
// }
