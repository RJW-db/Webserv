#include <HttpRequest.hpp>
#include <RunServer.hpp>

void    HttpRequest::POST()
{
    auto it = _client._headerFields.find("Content-Type");
    if (it == _client._headerFields.end())
        throw RunServers::ClientException("Missing Content-Type");

    getBodyInfo(_client._body);
    ContentType ct = getContentType(it->second);
    switch (ct) {
        case FORM_URLENCODED:
            // cout << "handle urlencoded" << endl;
            break;
        case JSON:
            // cout << "handle json" << endl;
            break;
        case TEXT:
            // cout << "handle text" << endl;
            break;
        case MULTIPART:
        {
            ofstream myfile;
            myfile.open("upload/" + string(_filename));
            std::cout << "writing " << std::endl;
            myfile << _fileContent;
            std::cout << "written to file" << std::endl;
            myfile.close();
            break;
        }
        default:
            throw RunServers::ClientException("Unsupported Content-Type: " + string(it->second));
    }

    string ok = "HTTP/1.1 200 OK\r\n";
    send(_client._fd, ok.c_str(), ok.size(), 0);
}

ContentType HttpRequest::getContentType(const string_view ct)
{
    if (ct == "application/x-www-form-urlencoded")
    {
        _contentType = ct;
        return FORM_URLENCODED;
    }
    if (ct == "application/json")
    {
        _contentType = ct;
        return JSON;
    }
    if (ct == "text/plain")
    {
        _contentType = ct;
        return TEXT;
    }
    if (ct.find("multipart/form-data") == 0)
    {
        size_t semi = ct.find(';');
        if (semi != std::string_view::npos)
        {
            _contentType = ct.substr(0, semi);
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != std::string_view::npos)
                _bodyBoundary = ct.substr(boundaryPos + 9); // 9 = strlen("boundary=")
            else
                throw RunServers::ClientException("Malformed multipart Content-Type: boundary not found");
        }
        else
            throw RunServers::ClientException("Malformed HTTP header line: " + string(ct));
        return MULTIPART;
    }
    return UNSUPPORTED;
}

void	HttpRequest::getBodyInfo(string &body)
{
    size_t cdPos = body.find("Content-Disposition:");
    if (cdPos == string::npos)
        throw RunServers::ClientException("Content-Disposition header not found in multipart body");

    // Extract the Content-Disposition line
    size_t cdEnd = body.find("\r\n", cdPos);
    string_view cdLine = string_view(body).substr(cdPos, cdEnd - cdPos);

    string filenameKey = "filename=\"";
    size_t fnPos = cdLine.find(filenameKey);
    if (fnPos != string::npos) {
        size_t fnStart = fnPos + filenameKey.size();
        size_t fnEnd = cdLine.find("\"", fnStart);
        _filename = cdLine.substr(fnStart, fnEnd - fnStart);
        if (_filename.empty())
            throw RunServers::ClientException("Filename is empty in Content-Disposition header");
    }
    else
        throw RunServers::ClientException("Filename not found in Content-Disposition header");

    const string contentType = "Content-Type: ";
    size_t position = body.find(contentType);

    if (position == string::npos)
        throw RunServers::ClientException("Content-Type header not found in multipart/form-data body part");

    size_t fileStart = body.find("\r\n\r\n", position) + 4;
    size_t fileEnd = body.find("\r\n--" + std::string(_bodyBoundary) /* + "--\r\n" */, fileStart);

    if (position == string::npos)
        throw RunServers::ClientException("Malformed or missing Content-Type header in multipart/form-data body part");

    _fileContent = string_view(body).substr(fileStart, fileEnd - fileStart);
}
