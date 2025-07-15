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
		HandleTransfer(Client &client, int fd, size_t bytesWritten, string boundaryBuffer); //post
        // HandleTransfer(Client &client); // POST
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other);
        ~HandleTransfer() = default;

        void readToBuf();
        bool handleGetTransfer();

        static bool foundBoundaryPost(Client &client, string &boundaryBuffer, int fd);
        static void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg, int fd);
        bool handlePostTransfer(bool ReadData = true);
        bool handleChunkTransfer();
        bool validateFinalCRLF();
        size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
        bool    searchContentDisposition();

        inline void setBoolToChunk() {
            _isChunked = true;
        }
        inline bool getIsChunk() const{
            return _isChunked;
        }
        // void setBoolToChunk();
        // bool getIsChunk();

        Client  &_client;
        int     _fd;
        string  _fileBuffer;
        size_t  _fileSize;

		

        size_t  _offset; // Offset for the data transfer
        size_t  _bytesReadTotal;
        size_t  _headerSize;
		
        bool    _epollout_enabled;
        bool    _foundBoundary;
        bool    _searchContentDisposition;
        
        

		size_t	_bytesWrittenTotal;

        // chunked
        bool   _isChunked;
        string _bodyHeader;
        size_t _bodyPos;
        size_t _chunkBodyPos;   
        // size_t _chunkPos;
        size_t _chunkTargetSize;
        size_t _chunkReceivedSize;
        string _unchunkedBody;


    protected:
        // Helper function to send data over a socket

};

#endif             