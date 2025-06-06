#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <unordered_map>
#include <map>
#include <string>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <fstream>
#include <sys/socket.h> //sockaddr
#include <netinet/in.h> //scokaddr_in
#include <iostream>
#include <vector>
#include <Location.hpp>

using namespace std;

class AconfigServ : public Aconfig
{
    public:
        AconfigServ &operator=(const AconfigServ &other);
        unordered_multimap<string, string> &getPortHost(void);
        unordered_map<string, Location> &getLocations(void);
        string &getServerName(void);

    protected:
        unordered_multimap<string, string> _portHost;
        unordered_map<string, Location> _locations;
        string _serverName; // if not found acts as default
        AconfigServ() = default;
        AconfigServ(const AconfigServ &other);
};

class ConfigServer : public AconfigServ
{
    public:
        ConfigServer();
        // ConfigServer(std::fstream &fs);
        ConfigServer(const ConfigServer &other);
        virtual ~ConfigServer();

        ConfigServer &operator=(const ConfigServer &other);

        bool listenHostname(string &line);
        bool serverName(string &line);

        void addLocation(const Location &location, string &path);

        void setDefaultConf(void);

        // map<uint16_t, string> ErrorCodesWithPage;
        private:
            uint32_t convertIpBinary(string ip);

            // vector<int> _listeners;
};




// string ftSkipspace(string &line);
void ftSkipspace(string &line);


#endif