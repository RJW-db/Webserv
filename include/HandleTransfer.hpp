#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include "Constants.hpp"
#include <string>
#include <stdexcept>

using namespace std;

class HandleShort
{
    public :
        Client &_client;
        int _fd;
        string _fileBuffer;
        size_t  _bytesReadTotal;


        // Pure virtual functions for polymorphic behavior
		virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not implemented"); }
		virtual bool handlePostTransfer(bool readData) { (void)readData; throw std::runtime_error("handlePostTransfer not implemented"); }
		virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not implemented"); }
		virtual bool getIsChunk() const { throw std::runtime_error("getIsChunk not implemented"); }
		virtual void appendToBody() { throw std::runtime_error("appendToBody not implemented"); }
        virtual void readFromCgi() { throw std::runtime_error("readFromCgi not implemented"); }
		virtual bool writeToCgi() { throw std::runtime_error("writeToCgi not implemented"); }
		virtual ~HandleShort() = default;

    protected :
        HandleShort(Client &client, int fd) : _client(client), _fd(fd) {};
};

class HandlePost : public HandleShort
{
public:
    HandlePost(Client &client, size_t bytesRead, string buffer); // POST
    HandlePost(const HandlePost &other) = default;
    HandlePost &operator=(const HandlePost &other);
    ~HandlePost() = default;

    // HandlePost specific members
    size_t _bytesWrittenTotal;

    bool _foundBoundary = false;
    bool _searchContentDisposition = false;

    vector<string> _fileNamePaths; // for post transfer - shared between HandlePost and HandleChunkPost

    bool handlePostTransfer(bool ReadData);

    void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);

    bool validateMultipartPostSyntax(Client &client, string &input);
    bool handlePostCgi();

protected:
    HandlePost(Client &client, int fd) : HandleShort(client, fd) {};

private:
    int validateFinalCRLF();
    size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
    bool searchContentDisposition();

    bool validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition);
    static void parseContentDisposition(Client &client, string_view &buffer);
};

class HandleGet : public HandleShort
{
    public :
        HandleGet(Client &client, int fd, string &responseHeader, size_t fileSize); // get
        HandleGet(const HandleGet &other) = default;
        HandleGet &operator=(const HandleGet &other);
        ~HandleGet() = default;

        void readToBuf();
        bool handleGetTransfer();

        size_t  _fileSize;
        size_t _offset;
        size_t  _headerSize;

        // Pure virtual function implementations - throw errors for unsupported operations
};


class HandleChunkPost : public HandlePost
{
    public :
        HandleChunkPost(Client &client); // chunked

        bool _isChunked = false;
        size_t _bodyPos = 0;
        bool _completedRequest = false;

        inline void setBoolToChunk()
        {
            _isChunked = true;
        }
        inline bool getIsChunk() const
        {
            return _isChunked;
        }
        void appendToBody();
        bool handleChunkTransfer();
        bool decodeChunk(size_t &chunkTargetSize);

        bool extractChunkSize(size_t &chunkTargetSize, size_t &chunkDataStart);
        void validateChunkSizeLine(string_view chunkSizeLine);
        uint64_t parseChunkSize(string_view chunkSizeLine);

    protected:
        // Helper function to send data over a socket

};

// class HandleTransferCgi :
// {
//     bool writeToCgi();
//     void readFromCgi();

// 	int _fd;

// };

class HandleTransferWriteCgi : public HandleShort
{
	public:
		HandleTransferWriteCgi(Client &client, int fdWriteToCgi, string &body);
		HandleTransferWriteCgi(const HandleTransferWriteCgi &other) = default;
		HandleTransferWriteCgi &operator=(const HandleTransferWriteCgi &other) = default;
		bool writeToCgi();
};

class HandleTransferRecvCgi : public HandleShort
{
	public:
		HandleTransferRecvCgi(Client &client, int fdReadFromCgi);
		HandleTransferRecvCgi(const HandleTransferRecvCgi &other) = default;
		HandleTransferRecvCgi &operator=(const HandleTransferRecvCgi &other) = default;
		void readFromCgi();
};
/*
        // CGI
        bool handleCgiTransfer();
        bool handlePostCgi();

        int8_t _isCgi = isNotCgi;

        int _fdWriteToCgi = -1;
        int _fdReadfromCgi = -1;

        inline int8_t getIsCgi() const {
            return _isCgi;
        }

*/
#endif