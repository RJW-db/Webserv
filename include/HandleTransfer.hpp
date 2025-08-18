#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include "Constants.hpp"
#include <string>
#include <stdexcept>

using namespace std;

class HandleTransfer
{
    public :
        Client &_client;
        int _fd;
        string _fileBuffer;
        size_t  _bytesReadTotal = 0;


        // Pure virtual functions for polymorphic behavior
		virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not implemented"); }
		virtual bool handlePostTransfer(bool readData) { (void)readData; throw std::runtime_error("handlePostTransfer not implemented"); }
		virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not implemented"); }
		virtual bool getIsChunk() const { throw std::runtime_error("getIsChunk not implemented"); }
		virtual void appendToBody() { throw std::runtime_error("appendToBody not implemented"); }
        virtual bool writeToCgiTransfer() { throw std::runtime_error("HandleCgitransfer not supported"); }
        virtual bool readFromCgiTransfer() { throw std::runtime_error("HandleCgitransfer not supported"); }
        virtual bool sendToClientTransfer() { throw std::runtime_error("sendToClientTransfer not supported"); }
        virtual ~HandleTransfer() = default;

    protected :
        HandleTransfer(Client &client, int fd) : _client(client), _fd(fd) {};
        // HandleToClientTransfer(Client &client, string &response);
        // HandleTransfer(Client &client, string &response);
        // HandleTransfer(Client &client, string &response) :_client(client), _fileBuffer(response) {};
};

class HandleGetTransfer : public HandleTransfer
{
    public :
        HandleGetTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
        HandleGetTransfer(const HandleGetTransfer &other) = default;
        HandleGetTransfer &operator=(const HandleGetTransfer &other);
        ~HandleGetTransfer() = default;

        void readToBuf();
        bool handleGetTransfer();

        size_t  _fileSize;
        size_t _offset;
        size_t  _headerSize;

        // Pure virtual function implementations - throw errors for unsupported operations
        virtual bool handlePostTransfer(bool readData) {(void)readData; throw std::runtime_error("handlePostTransfer not supported for HandleGetTransfer");}
        virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not supported for HandleGetTransfer");}
        virtual bool getIsChunk() const { throw std::runtime_error("getIsChunk not supported for HandleGetTransfer");}
        virtual void appendToBody() { throw std::runtime_error("appendToBody not supported for HandleGetTransfer");}
};

class HandlePostTransfer : public HandleTransfer
{
public:
    HandlePostTransfer(Client &client, size_t bytesRead, string buffer); // POST
    HandlePostTransfer(const HandlePostTransfer &other) = default;
    HandlePostTransfer &operator=(const HandlePostTransfer &other);
    ~HandlePostTransfer() = default;

        inline bool getIsChunk() const
        {
            return _isChunked;
        }

    bool _isChunked = false;


    // HandlePostTransfer specific members
    size_t _bytesWrittenTotal;

    bool _foundBoundary = false;
    bool _searchContentDisposition = false;

    vector<string> _fileNamePaths; // for post transfer - shared between HandlePostTransfer and HandleChunkTransfer

    bool handlePostTransfer(bool ReadData);

    void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);

    bool validateMultipartPostSyntax(Client &client, string &input);
    bool handlePostCgi();

protected:
    HandlePostTransfer(Client &client, int fd) : HandleTransfer(client, fd) {};

private:
    int validateFinalCRLF();
    size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
    bool searchContentDisposition();

    bool validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition);
    static void parseContentDisposition(Client &client, string_view &buffer);
};

class HandleChunkTransfer : public HandlePostTransfer
{
    public :
        HandleChunkTransfer(Client &client); // chunked

        size_t _bodyPos = 0;
        bool _completedRequest = false;

        inline void setBoolToChunk()
        {
            _isChunked = true;
        }

        virtual void appendToBody();
        virtual bool handleChunkTransfer();
        bool decodeChunk(size_t &chunkTargetSize);

        bool extractChunkSize(size_t &chunkTargetSize, size_t &chunkDataStart);
        void validateChunkSizeLine(string_view chunkSizeLine);
        uint64_t parseChunkSize(string_view chunkSizeLine);

    protected:
        // Helper function to send data over a socket

};

class HandleWriteToCgiTransfer : public HandleTransfer
{
    public:
        HandleWriteToCgiTransfer(Client &client, string &fileBuffer, int fdWriteToCgi);

        size_t _bytesWrittenTotal;


        bool writeToCgiTransfer();
        chrono::steady_clock::time_point _cgiDisconnectTime;
};

class HandleReadFromCgiTransfer : public HandleTransfer
{
    public:
        HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi);

        bool readFromCgiTransfer();
        chrono::steady_clock::time_point _cgiDisconnectTime;
};

class HandleToClientTransfer : public HandleTransfer
{
    public:
        HandleToClientTransfer(Client &client, string &response);

        bool sendToClientTransfer();
        // string &response;
};
#endif