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
        size_t  _bytesReadTotal;


        // Pure virtual functions for polymorphic behavior
        virtual bool handleGetTransfer() = 0;
        virtual bool handlePostTransfer(bool readData) = 0;
        virtual bool handleChunkTransfer() = 0;
        virtual bool getIsChunk() const = 0;
        virtual void appendToBody() = 0;
        virtual bool writeToCgiTransfer() {throw std::runtime_error("HandleCgitransfer not supported");}
        virtual bool readFromCgiTransfer() {throw std::runtime_error("HandleCgitransfer not supported");}
        virtual ~HandleTransfer() = default;

    protected :
        HandleTransfer(Client &client, int fd) : _client(client), _fd(fd) {};
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

    // HandlePostTransfer specific members
    size_t _bytesWrittenTotal;

    bool _foundBoundary = false;
    bool _searchContentDisposition = false;

    vector<string> _fileNamePaths; // for post transfer - shared between HandlePostTransfer and HandleChunkTransfer

    bool handlePostTransfer(bool ReadData);

    void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);

    bool validateMultipartPostSyntax(Client &client, string &input);
    bool handlePostCgi();

    // Pure virtual function implementations - throw errors for unsupported operations
    virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not supported for HandlePostTransfer");}
    virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not supported for HandlePostTransfer");}
    virtual bool getIsChunk() const { return false;}
    virtual void appendToBody() { throw std::runtime_error("appendToBody not supported for HandlePostTransfer");}

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

class HandleWriteToCgiTransfer : public HandleTransfer
{
    public:
        HandleWriteToCgiTransfer(Client &client, string &fileBuffer, int fdWriteToCgi);

        size_t _bytesWrittenTotal;

        bool writeToCgiTransfer();

        virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool handlePostTransfer(bool readData) { (void)readData; throw std::runtime_error("handlePostTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool getIsChunk() const { return false; }
        virtual void appendToBody() { throw std::runtime_error("appendToBody not supported for HandleWriteToCgiTransfer"); }

};

class HandleReadFromCgiTransfer : public HandleTransfer
{
    public:
        HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi);


        bool readFromCgiTransfer();

        virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool handlePostTransfer(bool readData) { (void)readData; throw std::runtime_error("handlePostTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not supported for HandleWriteToCgiTransfer"); }
        virtual bool getIsChunk() const { return false; }
        virtual void appendToBody() { throw std::runtime_error("appendToBody not supported for HandleWriteToCgiTransfer"); }

};

#endif