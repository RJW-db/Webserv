#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>

ContentType HttpRequest::getContentType(Client &client)
{
    auto it = client._headerFields.find("Content-Type");
    if (it == client._headerFields.end())
        throw RunServers::ClientException("Missing Content-Type");
    const string_view ct = it->second;
    if (ct == "application/x-www-form-urlencoded")
    {
        client._contentType = ct;
        return FORM_URLENCODED;
    }
    if (ct == "application/json")
    {
        client._contentType = ct;
        return JSON;
    }
    if (ct == "text/plain")
    {
        client._contentType = ct;
        return TEXT;
    }
    if (ct.find("multipart/form-data") == 0)
    {
        size_t semi = ct.find(';');
        if (semi != string_view::npos)
        {
            client._contentType = ct.substr(0, semi);
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != string_view::npos)
                client._bodyBoundary = ct.substr(boundaryPos + 9); // 9 = strlen("boundary=")
            else
                throw RunServers::ClientException("Malformed multipart Content-Type: boundary not found");
        }
        else
            throw RunServers::ClientException("Malformed HTTP header line: " + string(ct));
        return MULTIPART;
    }
    return UNSUPPORTED;
}

void HttpRequest::getBodyInfo(Client &client)
{
    size_t cdPos = client._body.find("Content-Disposition:");
    if (cdPos == string::npos)
        throw RunServers::ClientException("Content-Disposition header not found in multipart body");

    // Extract the Content-Disposition line
    size_t cdEnd = client._body.find("\r\n", cdPos);
    string_view cdLine = string_view(client._body).substr(cdPos, cdEnd - cdPos);

    string filenameKey = "filename=\"";
    size_t fnPos = cdLine.find(filenameKey);
    if (fnPos != string::npos)
    {
        size_t fnStart = fnPos + filenameKey.size();
        size_t fnEnd = cdLine.find("\"", fnStart);
        client._filename = cdLine.substr(fnStart, fnEnd - fnStart);
        if (client._filename.empty())
            throw RunServers::ClientException("Filename is empty in Content-Disposition header");
    }
    else
        throw RunServers::ClientException("Filename not found in Content-Disposition header");

    const string contentType = "Content-Type: ";
    size_t position = client._body.find(contentType);

    if (position == string::npos)
        throw RunServers::ClientException("Content-Type header not found in multipart/form-data body part");

    size_t fileStart = client._body.find("\r\n\r\n", position) + 4;
    // size_t fileEnd = client._body.find("\r\n--" + string(client._bodyBoundary) /* + "--\r\n" */, fileStart);

    // if (position == string::npos)
    //     throw RunServers::ClientException("Malformed or missing Content-Type header in multipart/form-data body part");

    // client._fileContent = string_view(client._body).substr(fileStart, fileEnd - fileStart);
}




void HttpRequest::validateChunkSizeLine(const string &input)
{
    if (input.size() < 1)
    {
        cout << "wrong1" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }

    // size_t hexPart = input.size() - 2;
    if (all_of(input.begin(), input.end(), ::isxdigit) == false)
    {
        cout << "wrong2" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }
        
    // if (input[hexPart] != '\r' && input[hexPart + 1] != '\n')
    // {
    //     cout << "wrong3" << endl; //testcout
    //     if (input[hexPart] == '\r')
    //         cout << "\\r" << endl; //testcout
    //     if (input[hexPart + 1] == '\n')
    //         cout << "\\n" << endl; //testcout
    //     // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    // }
}
#include <string>
uint64_t HttpRequest::parseChunkSize(const string &input)
{
    uint64_t chunkSize;
    stringstream ss;
    ss << hex << input;
    ss >> chunkSize;
    // cout << "chunkSize " << chunkSize << endl;
    // if (static_cast<size_t>(chunkSize) > client._location.getClientBodySize())
    // {
    //     throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkSize)); // (413, "Payload Too Large");
    // }
    return chunkSize;
}

void HttpRequest::ParseChunkStr(const string &input, uint64_t chunkSize)
{
    if (input.size() - 2 != chunkSize)
    {
        cout << "test1" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Chunk data does not match declared chunk size");
    }

    if (input[chunkSize] != '\r' || input[chunkSize + 1] != '\n')
    {
        cout << "test2" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "chunk data missing CRLF");
    }
}


void HttpRequest::handleChunks(Client &client)
{
    string line = client._body.substr(client._chunkPos);
    // std::cout << escape_special_chars(line) <<endl; //testcout
    size_t crlf = line.find("\r\n");
    if (crlf == string::npos)
    {
        std::cout << "\nhandleChunks 1" << std::endl; //testcout
        return;
    }
    std::cout << "\n\n" << escape_special_chars(line) <<endl<<endl; //testcout
    string chunkSizeLine = line.substr(0, crlf);
    std::cout << escape_special_chars(chunkSizeLine) << endl; //testcout
    validateChunkSizeLine(chunkSizeLine);
    
    uint64_t chunkSize = parseChunkSize(chunkSizeLine);
    std::cout << "chunkSize = " << chunkSize << endl; //testcout

    size_t chunkDataCrlf = line.find("\r\n", crlf + 2);
    if (chunkDataCrlf == string::npos)
    if (crlf == string::npos)
    {
        std::cout << "\nhandleChunks 2" << std::endl; //testcout
        return; // but should start looking for chunkData and not chunkSize
    }
    string chunkData = line.substr(crlf + 2, chunkDataCrlf - (crlf + 2));
    std::cout << escape_special_chars(chunkData) << std::endl; //testcout

    std::cout << "chunkData = " << chunkData.size() << std::endl; //testcout

    std::cout << "<" <<client._bodyBoundary << ">"<< std::endl; //testcout
    if (string("--") + string(client._bodyBoundary) == chunkData)
    {
        std::cout << "worked well" << std::endl; //testcout
    }
    // if (chunkData[39] == '\r')
    //     std::cout << "made it" << std::endl; //testcout
    // std::cout << chunkData[39] << std::endl; //testcout
    exit(0);
}


// first parse body header till \r\n\r\n
// '------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="upload.txt"\r\nContent-Type: text/plain\r\n\r\n'