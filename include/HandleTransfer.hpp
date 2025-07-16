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


struct ChunkState
{
    string _body;
    string _bodyHeader;
    size_t _bodyPos = 0;
    size_t _chunkBodyPos = 0;
    size_t _chunkTargetSize = 0;
    size_t _chunkReceivedSize = 0;
    string _unchunkedBody;

    
    bool   _foundBoundary = false;

    void reset()
    {
        _bodyHeader.clear();
        _bodyPos = 0;
        _chunkBodyPos = 0;
        _chunkTargetSize = 0;
        _chunkReceivedSize = 0;
        _unchunkedBody.clear();
        _foundBoundary = false;
    }
};

class HandleTransfer : protected ChunkState
{
    public:
        HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
		HandleTransfer(Client &client, int fd, size_t bytesWritten, string buffer); //post
        HandleTransfer(Client &client, string body); // POST
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other);
        ~HandleTransfer() = default;

        void readToBuf();
        bool handleGetTransfer();

        static void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg, int fd);
        bool handlePostTransfer(bool ReadData = true);
        bool validateFinalCRLF();
        size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
        bool    searchContentDisposition();


        Client  &_client;
        int     _fd; // targetFilePathFD, rename this for merging
        string  _fileBuffer;
        size_t  _fileSize;

		

        size_t  _offset = 0; // Offset for the data transfer
        size_t  _bytesReadTotal = 0;
        size_t  _headerSize;
		
        bool    _epollout_enabled = true;
        bool    _foundBoundary = false;
        bool    _searchContentDisposition = false;
        
		size_t	_bytesWrittenTotal;

        // chunked
        bool   _isChunked = false;

        inline void setBoolToChunk() {
            _isChunked = true;
        }
        inline bool getIsChunk() const{
            return _isChunked;
        }
        
        void appendToBody();
        bool handleChunkTransfer();
        bool processChunkBodyHeader();

        bool extractChunkSize();
        static void validateChunkSizeLine(const string &input);
        static uint64_t parseChunkSize(const string &input);
        static void ParseChunkStr(const string &input, uint64_t chunkTargetSize);



    protected:
        // Helper function to send data over a socket

};

#endif