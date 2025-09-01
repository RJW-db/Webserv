#include "ConfigServer.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"

ConfigServer::ConfigServer()
{
}

ConfigServer::ConfigServer(const ConfigServer &other) : AconfigServ(other)
{
    *this = other;
}

ConfigServer::~ConfigServer(){ }

ConfigServer &ConfigServer::operator=(const ConfigServer &other)
{
    if (this != &other)
    {
        AconfigServ::operator=(other);
    }
    return (*this);
}

bool ConfigServer::listenHostname(string &line)
{ 
    // to do should we store in addrinfo immediately? and how to handle fstream
    size_t skipHostname = line.find_first_not_of("0123456789.");
    size_t index =  line.find_first_not_of(" \t\f\v\r0123456789.");
    string hostname = "0.0.0.0";
    if (index == string::npos || line[index] == ';' )
    {
        if (line.find('.') < skipHostname)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "listen: invalid hostname - '", line, "'");
    }
    else if (line[index] == ':')
    {
        hostname = line.substr(0, skipHostname);
        line.erase(0, skipHostname + 1);
    }
    else
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "listen: invalid character found after hostname - '", line, "'");
    uint16_t port = static_cast<uint16_t>(stoul(line, &index));
    if (port == 0)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "listen: invalid port entered for listen should be between 1 and 65535: ", +port);
    string strPort = line.substr(0, index);
    for (const auto& pair : _portHost)
    {
        if (pair.first == line.substr(0, index) && pair.second == hostname)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "listen: tried setting same port and hostname twice: ", hostname, ':', strPort);
    }
    _portHost.insert({strPort, hostname});
    return (handleNearEndOfLine(line, index, "listen"));
}

bool ConfigServer::serverName(string &line)
{
    if (!_serverName.empty())
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "server_name: Parsing: tried setting server_name twice");
    size_t len = line.find_first_of(" \t\f\v\r;");
    if (len == string::npos)
    {
        _serverName = line;
        return (false);
    }
    _serverName = line.substr(0, len);
    return (handleNearEndOfLine(line, len, "server_name"));
}

string &ConfigServer::getServerName() { return _serverName; }

void ConfigServer::addLocation(const Location &location, string path)
{
    for (auto it = _locations.begin(); it != _locations.end(); ++it)
    {
        pair<string, Location> &val = *it;
        if (val.first == path)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "addLocation: Parsing: tried adding location with same path twice: ", path);
        if (val.first.length() < path.length())
        {
            _locations.insert(it, {path, location});
            return;
        }
    }
    _locations.insert(_locations.end(), {path, location});
}

int ConfigServer::getLineNbr() const { return _lineNbr; }

void ConfigServer::setDefaultConf()
{
    _root.insert(0, RunServers::getServerRootDir());
    if (_root.back() == '/')
        _root = _root.substr(0, _root.size() - 1);
    
    if (_clientMaxBodySize == 0)
        _clientMaxBodySize = 1024 * 1024;
    if (_autoIndex == autoIndexNotFound)
        _autoIndex = autoIndexFalse;
    if (_serverName.empty())
    {
        static size_t serverIndex = 0;
        _serverName = "Server" + to_string(++serverIndex);
    }
    if (_portHost.empty())
    {
        string tmp = "80;";
        listenHostname(tmp); // sets sockaddr ip to 0.0.0.0 and port to 80
    }
    for (auto &val : _locations)
    {
        Location &location = val.second;
        location.SetDefaultLocation(*this);
    }
    bool found = false;
    for (auto it = _locations.begin(); it != _locations.end(); ++it)
    {
        if (it->first == "/")
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        Location location;
        location.SetDefaultLocation(*this);
        _locations.insert(_locations.begin(), {"/", location});
    }
    setDefaultErrorPages();
}

AconfigServ::AconfigServ(const AconfigServ &other) : Aconfig(other) { *this = other; }

AconfigServ &AconfigServ::operator=(const AconfigServ &other) {
    if (this != &other) {
        Aconfig::operator=(other);
        _portHost = other._portHost;
        _locations = other._locations;
        _serverName = other._serverName;
    }
    return *this;
}

unordered_multimap<string, string> &AconfigServ::getPortHost() { return _portHost; }
vector<pair<string, Location>> &AconfigServ::getLocations() { return _locations; }
string &AconfigServ::getServerName() { return _serverName; }
