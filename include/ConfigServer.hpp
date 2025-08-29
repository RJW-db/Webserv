#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP
#include <unordered_map>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include "Location.hpp"
using namespace std;

class AconfigServ : public Aconfig
{
    public:
        //initialization
        AconfigServ() = default;
        AconfigServ(const AconfigServ &other);
        AconfigServ &operator=(const AconfigServ &other);

        //getters
        unordered_multimap<string, string> &getPortHost(void);
        vector<pair<string, Location>> &getLocations(void);
        string &getServerName(void);
        
    protected:
        //values to be stored
        unordered_multimap<string, string> _portHost;
        vector<pair<string, Location>> _locations;
        string _serverName; // if not found acts as default
};

class ConfigServer : public AconfigServ
{
    public:
        //initialization
        ConfigServer();
        ConfigServer(const ConfigServer &other);
        virtual ~ConfigServer();
        ConfigServer &operator=(const ConfigServer &other);

        //parsing logic
        bool listenHostname(string &line);
        bool serverName(string &line);
        string &getServerName(void);

        //utils
        void addLocation(const Location &location, string path);
        int getLineNbr(void) const;

        //default setting
        void setDefaultConf(void);
};
#endif
