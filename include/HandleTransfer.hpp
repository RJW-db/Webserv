#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include <string>

using namespace std;
// read in stukken kunnen lezen van fd
// een class/struct met 
// fd
// buffer(geallocated)
// index_ofsset

// kan fd sluiten nadat read klaar is.

// en dan send return opslaan in index_ofset om het in correcte delen te versturen.
class HandleTransfer
{
    public:
        HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize);
		HandleTransfer(Client &client, int fd, size_t bytesWritten, size_t finalFileSize, string boundary);
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other);
        virtual ~HandleTransfer() = default;
        // static bool foundBoundaryPost(Client &client, string rootPath, int fd);
        bool handleGetTransfer();
        bool handlePostTransfer();

        Client  &_client;
        int     _fd;
        string  _fileBuffer;
        size_t  _fileSize;

		

        size_t  _offset; // Offset for the data transfer
        size_t  _bytesReadTotal;
        size_t  _headerSize;
		
        bool    _epollout_enabled;

		size_t	_bytesWrittenTotal;
    protected:
        // Helper function to send data over a socket

};

#endif             