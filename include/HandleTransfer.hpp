#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include "Constants.hpp"
#include <string>

using namespace std;

class HandleTransfer
{
    public:
        HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
		HandleTransfer(Client &client, size_t bytesRead, string buffer); //POST
        HandleTransfer(Client &client); // POST chunked
        HandleTransfer(const HandleTransfer &other) = default;
        HandleTransfer &operator=(const HandleTransfer &other);
        ~HandleTransfer() = default;

        void readToBuf();
        bool handleGetTransfer();

        // static bool foundBoundaryPost(Client &client, string &boundaryBuffer, int fd);
        void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);
        bool handlePostTransfer(bool ReadData);
        int validateFinalCRLF();
        size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
        bool    searchContentDisposition();

        Client  &_client;
        int     _fd; // targetFilePathFD, rename this for merging
        string  _fileBuffer;
        size_t  _fileSize;

        vector<string> _fileNamePaths; // for post transfer

        size_t  _offset = 0; // Offset for the data transfer
        size_t  _bytesReadTotal = 0;
        size_t  _headerSize;
		
        bool    _epollout_enabled = true;
        bool    _foundBoundary = false;
        bool    _searchContentDisposition = false;
        
		size_t	_bytesWrittenTotal;

        // chunked
        bool   _isChunked = false;
        size_t _bodyPos = 0;
        bool   _completedRequest = false;

        inline void setBoolToChunk() {
            _isChunked = true;
        }
        inline bool getIsChunk() const {
            return _isChunked;
        }
        void appendToBody();
        bool handleChunkTransfer();
        bool decodeChunk(size_t &chunkTargetSize);

        bool extractChunkSize(size_t &chunkTargetSize, size_t &chunkDataStart);
        void validateChunkSizeLine(string_view chunkSizeLine);
        uint64_t parseChunkSize(string_view chunkSizeLine);

        bool validateMultipartPostSyntax(Client &client, string &input);
        bool validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition);
        static void parseContentDisposition(Client &client, string_view &buffer);





    protected:
        // Helper function to send data over a socket

};

#endif