#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP
#include <unordered_map>
#include <string>
#include <vector>
#include "Location.hpp"
using namespace std;

class AconfigServ : public Aconfig
{
    public:
        // Initialization
        AconfigServ() = default;
        AconfigServ(const AconfigServ &other);
        AconfigServ &operator=(const AconfigServ &other);

        // Getters
        unordered_multimap<string, string> &getPortHost(void);
        vector<pair<string, Location>> &getLocations(void);
        string &getServerName(void);
        
    protected:
        unordered_multimap<string, string> _portHost;
        vector<pair<string, Location>> _locations;
        string _serverName; // if not found acts as default
};

class ConfigServer : public AconfigServ
{
    public:
        // Initialization
        ConfigServer();
        ConfigServer(const ConfigServer &other);
        virtual ~ConfigServer();
        ConfigServer &operator=(const ConfigServer &other);

        // Parsing logic
        bool listenHostname(string &line);
        bool serverName(string &line);
        string &getServerName(void);

        // Utils
        void addLocation(const Location &location, string path);
        int getLineNbr(void) const;

        // Default setting
        void setDefaultConf(void);
};
#endif
