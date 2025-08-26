#ifndef HANDLETRANSFER_HPP
#define HANDLETRANSFER_HPP

#include <Client.hpp>
#include <Constants.hpp>
#include <string>
#include <Logger.hpp>
#include <stdexcept>

using namespace std;

enum HandleTransferType {
    HANDLE_GET_TRANSFER = 1,
    HANDLE_POST_TRANSFER = 2,
    HANDLE_CHUNK_TRANSFER = 3,
    HANDLE_WRITE_TO_CGI_TRANSFER = 4,
    HANDLE_READ_FROM_CGI_TRANSFER = 5,
    HANDLE_TO_CLIENT_TRANSFER = 6
};

enum ValidationResult {
    CONTINUE_READING = 0,
    FINISHED = 1,
    RERUN_WITHOUT_READING = 2
};

class HandleTransfer
{
    public :
        Client &_client;
        int _fd;
        string _fileBuffer;
        size_t  _bytesReadTotal;
        HandleTransferType _handleType;

        // Pure virtual functions for polymorphic behavior
		virtual bool handleGetTransfer() { throw std::runtime_error("handleGetTransfer not implemented for handle: " + to_string(_handleType)); }
		virtual bool postTransfer(bool readData) { (void)readData; throw std::runtime_error("handlePostTransfer not implemented for handle: " + to_string(_handleType)); }
		virtual bool handleChunkTransfer() { throw std::runtime_error("handleChunkTransfer not implemented for handle: " + to_string(_handleType)); }
		virtual bool getIsChunk() const { throw std::runtime_error("getIsChunk not implemented for handle: " + to_string(_handleType)); }
		virtual void appendToBody() { throw std::runtime_error("appendToBody not implemented for handle: " + to_string(_handleType)); }
        virtual bool writeToCgiTransfer() { throw std::runtime_error("HandleCgitransfer not supported for handle: " + to_string(_handleType)); }
        virtual bool readFromCgiTransfer() { throw std::runtime_error("HandleCgitransfer not supported for handle: " + to_string(_handleType)); }
        virtual bool sendToClientTransfer() { throw std::runtime_error("sendToClientTransfer not supported for handle: " + to_string(_handleType)); }
        virtual ~HandleTransfer() = default;

    protected :
        HandleTransfer(Client &client, int fd, HandleTransferType handleType) : _client(client), _fd(fd), _handleType(handleType) {};
};

class HandleGetTransfer : public HandleTransfer
{
    public :
    // initialization
        HandleGetTransfer(Client &client, int fd, string &responseHeader, size_t fileSize); // get
        ~HandleGetTransfer() {
            FileDescriptor::closeFD(_fd);
        };

        //logic functions
        bool handleGetTransfer();
        void fileReadTobuff();

        //variables needed
        size_t  _fileSize;
        size_t _offset;
        size_t  _headerSize;
};

class HandlePostTransfer : public HandleTransfer
{
    public:
        //initialization
        HandlePostTransfer(Client &client, size_t bytesRead, string buffer); // POST
        ~HandlePostTransfer() = default;

        // handle function
        virtual bool postTransfer(bool readData);

        // HandlePostTransfer specific members
        size_t _bytesWrittenTotal;
        bool _foundBoundary = false;
        bool _searchContentDisposition = false;
        vector<string> _fileNamePaths; // for post transfer - shared between HandlePostTransfer and HandleChunkTransfer

    private:
        // recv incoming
        void ReadIncomingData();

        //process
        bool processMultipartData();
        size_t FindBoundaryAndWrite(ssize_t &bytesWritten);
        bool searchContentDisposition();
        ValidationResult validateFinalCRLF();

        // success
        void sendSuccessResponse();

        // cgi
        bool handlePostCgi();

    protected:
        // HandlePostTransfer specific constructor for derived classes
        HandlePostTransfer(Client &client, int fd) : HandleTransfer(client, fd, HANDLE_POST_TRANSFER) {};

        //util function
        void errorPostTransfer(Client &client, uint16_t errorCode, string errMsg);
};

class HandleChunkTransfer : public HandlePostTransfer
{
    public :
        HandleChunkTransfer(Client &client); // chunked

        size_t _bodyPos = 0;
        bool _completedRequest = false;

        // inline void setBoolToChunk()
        // {
        //     _isChunked = true;
        // }

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

class MultipartParser
{
public:
    static bool validateMultipartPostSyntax(Client &client, string &input);
    static bool validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition);
    static void parseContentDisposition(Client &client, string_view &buffer);

private:
    MultipartParser() = default; // Prevent instantiation since this is a utility class
};

#endif