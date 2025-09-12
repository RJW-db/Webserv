#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP
#include <stdexcept>
#include <string>
#include <cstdint>
#include "FileDescriptor.hpp"
using namespace std;
#ifndef CLIENT_BUFFER_SIZE
# define CLIENT_BUFFER_SIZE 8192 // 8KB
#endif
class Client;
enum HandleTransferType : uint8_t
{
    HANDLE_GET_TRANSFER = 1,
    HANDLE_POST_TRANSFER = 2,
    HANDLE_CHUNK_TRANSFER = 3,
    HANDLE_WRITE_TO_CGI_TRANSFER = 4,
    HANDLE_READ_FROM_CGI_TRANSFER = 5,
    HANDLE_TO_CLIENT_TRANSFER = 6
};

enum ValidationResult : uint8_t
{
    CONTINUE_READING = 0,
    FINISHED = 1,
    RERUN_WITHOUT_READING = 2
};

class HandleTransfer
{
    public:
        virtual bool handleGetTransfer() { throw runtime_error("handleGetTransfer not implemented for handle: " + to_string(_handleType)); }
        virtual bool postTransfer(bool readData) { (void)readData; throw runtime_error("handlePostTransfer not implemented for handle: " + to_string(_handleType)); }
        virtual bool handleChunkTransfer() { throw runtime_error("handleChunkTransfer not implemented for handle: " + to_string(_handleType)); }
        virtual void appendToBody() { throw runtime_error("appendToBody not implemented for handle: " + to_string(_handleType)); }
        virtual bool writeToCgiTransfer() { throw runtime_error("HandleCgitransfer not supported for handle: " + to_string(_handleType)); }
        virtual bool readFromCgiTransfer() { throw runtime_error("HandleCgitransfer not supported for handle: " + to_string(_handleType)); }
        virtual bool sendToClientTransfer() { throw runtime_error("sendToClientTransfer not supported for handle: " + to_string(_handleType)); }
        virtual ~HandleTransfer() = default;

        Client &_client;
        int _fd;
        string _fileBuffer;
        size_t  _bytesReadTotal = 0;
        HandleTransferType _handleType;

    protected:
        HandleTransfer(Client &client, int fd, HandleTransferType handleType) : _client(client), _fd(fd), _handleType(handleType) {};
};

class HandleGetTransfer : public HandleTransfer
{
    public:
        HandleGetTransfer(Client &client, int fd, const string &responseHeader, size_t fileSize, size_t headerSize); // get
        ~HandleGetTransfer() override { FileDescriptor::closeFD(_fd); };

        // Main logic
        bool handleGetTransfer() override;
        void fileReadToBuff();

        size_t  _fileSize;
        size_t _headerSize;
        size_t _offset;
        size_t _bytesSentTotal = 0;
};

class HandlePostTransfer : public HandleTransfer
{
    public:
        HandlePostTransfer(Client &client, size_t bytesRead, string buffer); // POST

        // Main logic
        virtual bool postTransfer(bool readData) override;

        size_t _bytesWrittenTotal = 0;
        bool _foundBoundary = false;
        bool _searchContentDisposition = false;
        vector<string> _fileNamePaths; // For post transfer - shared between HandlePostTransfer and HandleChunkTransfer

        bool _completedChunkedRequest = false;

    protected:
        // Protected constructor for derived classes
        HandlePostTransfer(Client &client, int fd) : HandleTransfer(client, fd, HANDLE_POST_TRANSFER) {};

        // Utility
        void errorPostTransfer(Client &client, uint16_t errorCode, const string &errMsg);

    private:
        // recv incoming
        void ReadIncomingData();

        // Process
        bool processMultipartData();
        size_t FindBoundaryAndWrite(size_t &bytesWritten);
        bool searchContentDisposition();
        ValidationResult validateFinalCRLF();
        void sendSuccessResponse();
        bool handlePostCgi();
};

class HandleChunkTransfer : public HandlePostTransfer
{
    public:
        explicit HandleChunkTransfer(Client &client);

        // Logic functions
        virtual void appendToBody() override;
        virtual bool handleChunkTransfer() override;
        bool decodeChunk();

        // Chunk parsing helpers
        bool extractChunkSize(size_t &chunkTargetSize, size_t &chunkDataStart);
        void validateChunkSizeLine(string_view chunkSizeLine);
        uint64_t parseChunkSize(string_view chunkSizeLine);
        
        size_t _bodyPos = 0;
};

class HandleWriteToCgiTransfer : public HandleTransfer
{
    public:
        HandleWriteToCgiTransfer(Client &client, const string &fileBuffer, int fdWriteToCgi);
        ~HandleWriteToCgiTransfer() override { FileDescriptor::cleanupEpollFd(_fd); };

        bool writeToCgiTransfer() override;

        size_t _bytesWrittenTotal = 0;
};

class HandleReadFromCgiTransfer : public HandleTransfer
{
    public:
        HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi);
        ~HandleReadFromCgiTransfer() override { FileDescriptor::cleanupEpollFd(_fd); };

        bool readFromCgiTransfer() override;
};

class HandleToClientTransfer : public HandleTransfer
{
    public:
        HandleToClientTransfer(Client &client, const string &response);
        ~HandleToClientTransfer() override { FileDescriptor::cleanupEpollFd(_fd); };

        bool sendToClientTransfer() override;
};

class MultipartParser
{
    public:
        static void validateMultipartPostSyntax(Client &client, string &input);
        static bool validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition);
        static void parseContentDisposition(Client &client, string_view &buffer);

    private:
        MultipartParser() = default; // Prevent instantiation since this is a utility class
};
#endif
