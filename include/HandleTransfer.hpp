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
        HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
		HandleTransfer(Client &client, int fd, size_t bytesRead, string buffer); //post
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other);
        ~HandleTransfer() = default;

        void readToBuf();
        bool handleGetTransfer();

        static bool foundBoundaryPost(Client &client, string &boundaryBuffer, int fd);
        void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);
        bool handlePostTransfer(bool ReadData = true);
        int validateFinalCRLF();
        size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
        bool    searchContentDisposition();

        Client  &_client;
        int     _fd;
        string  _fileBuffer;
        size_t  _fileSize;

		map<string, string> _arguments; // for post transfer
        vector<string> _fileNamePaths; // for post transfer

        size_t  _offset; // Offset for the data transfer
        size_t  _bytesReadTotal;
        size_t  _headerSize;
		
        bool    _epollout_enabled;
        bool    _foundBoundary;
        bool    _searchContentDisposition;
        
        

		size_t	_bytesWrittenTotal;
    protected:
        // Helper function to send data over a socket

};

#endif             