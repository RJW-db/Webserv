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
        HandleTransfer(int clientFD, string &responseHeader, int fd);
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other) = default;
        virtual ~HandleTransfer() = default;

        // Function to handle the transfer of data


        int     _clientFD;
        string  _header;
        size_t _offset; // Offset for the data transfer
        
        int     _fd;
        string  _fileBuffer;

        bool    _epollout_enabled;
    protected:
        // Helper function to send data over a socket

};

#endif             