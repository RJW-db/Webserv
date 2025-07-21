#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include "Constants.hpp"
#include <string>

using namespace std;

struct ChunkState
{
    // string _body;
    string _bodyHeader;
    size_t _bodyPos = 0;
    size_t _boundaryPos = 0;
    size_t _chunkDataStart = 0;
    size_t _chunkTargetSize = 0;
    size_t _chunkReceivedSize = 0;
    string _unchunkedBody;        // Contains full unchunked data including boundaries
    string _fileContent;          // Contains only the actual file data (clean)

    size_t _searchStartPos = 0;

    bool   _foundBoundary = false;

    void reset()
    {
        _bodyHeader.clear();
        _bodyPos = 0;
        _boundaryPos = 0;
        _chunkDataStart = 0;
        _chunkTargetSize = 0;
        _chunkReceivedSize = 0;
        _unchunkedBody.clear();
        _fileContent.clear();
        _foundBoundary = false;
    }
};

class HandleTransfer : protected ChunkState
{
    public:
        HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
		HandleTransfer(Client &client, int fd, size_t bytesWritten, string buffer); //post
        HandleTransfer(Client &client); // POST
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
        inline bool getIsChunk() const {
            return _isChunked;
        }
        inline bool hasBoundaryAt(const string &_unchunkedBody, size_t pos, string_view boundary) {
            return pos + BOUNDARY_PREFIX_LEN + boundary.size() <= _unchunkedBody.size() &&
                   _unchunkedBody[pos] == '-' &&
                   _unchunkedBody[pos + 1] == '-' &&
                   (_unchunkedBody.compare(pos + BOUNDARY_PREFIX_LEN, boundary.size(),
                                 boundary.data(), boundary.size()) == 0);
        }
        void appendToBody();
        bool handleChunkTransfer();
        bool processBodyHeader(size_t crlf2Pos);
        bool decodeChunk();
        bool FinalCrlfCheck();

        bool extractChunkSize();
        static void validateChunkSizeLine(const string &input);
        static uint64_t parseChunkSize(const string &input);
        static void ParseChunkStr(const string &input, uint64_t chunkTargetSize);



    protected:
        // Helper function to send data over a socket

};

#endif