#include <RunServer.hpp>
#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <unistd.h>
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

HandleTransferWriteCgi::HandleTransferWriteCgi(Client &client, int fdWriteToCgi, string &body)
: HandleShort(client, fdWriteToCgi)
{
	_fileBuffer = body;
}

bool HandleTransferWriteCgi::writeToCgi()
{
	ssize_t written = write(_fd, _fileBuffer.data(), _fileBuffer.size());
	if (written == -1)
		throw ErrorCodeClientException(_client, 500, "Couldn't Write to cgi process");
	_fileBuffer.erase(0, written);
	return (_fileBuffer.empty());
}

HandleTransferRecvCgi::HandleTransferRecvCgi(Client &client, int fdReadFromCgi)
: HandleShort(client, fdReadFromCgi)
{
}

void HandleTransferRecvCgi::readFromCgi()
{
	char buff[RunServers::getClientBufferSize()];
	ssize_t bytesRead = read(_fd, buff, RunServers::getClientBufferSize());
	if (bytesRead == -1)
		throw ErrorCodeClientException(_client, 500, "couldn't read from cgi process");
	ssize_t bytesSent = send(_client._fd, buff, bytesRead, 0);
	if (bytesSent != bytesRead)
	{
		std::cout << "bytes sent lower then bytes read from cgi process" << std::endl; // testcout
	}
}


// void HandleTransferCgi::writeToCgi()
// {

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
