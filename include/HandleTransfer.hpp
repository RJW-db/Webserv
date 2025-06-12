#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

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
        HandleTransfer(int clientFD, string &responseHeader, int fd, size_t fileSize, string &boundary);
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other) = default;
        virtual ~HandleTransfer() = default;

        int     _clientFD;
        int     _fd;
        size_t  _fileSize;
        string  _fileBuffer;
        string  _boundary;

        size_t _offset; // Offset for the data transfer
        size_t  _bytesReadTotal;

        bool    _epollout_enabled;
    protected:
        // Helper function to send data over a socket

};

#endif             