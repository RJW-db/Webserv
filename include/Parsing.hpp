#ifndef PARSING_HPP
#define PARSING_HPP
#include <string>
#include <vector>
#include "ConfigServer.hpp"
using namespace std;

class Parsing
{
    public:
        // Initialization
        explicit Parsing(const char *input);
        ~Parsing() = default;

        // Getters
        vector<ConfigServer> &getConfigs() {
            return _configs;
        }

    private:
        // Helper methods for better organization
        void readConfigFile(const char *input);

        // Parsing
        void processServerBlocks();
        template <typename T>
            void readBlock(T &block, 
                const map<string, bool (T::*)(string&)> &cmds, 
                const map<string, bool (T::*)(string &)> &whileCmds);

        // Checking correct syntax functions
        void ServerCheck();
        template <typename T>
            void cmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);
        template <typename T>
            void whileCmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);
        template <typename T>
            void LocationCheck(string &line, T &block);
        bool    checkParseSyntax(void);

        // Line handling functions
        template <typename T>
        void skipLine(string &line, bool forceSkip, T &curConf, bool skipSpace);

        vector<ConfigServer> _configs;
        map<int, string> _lines;
        bool _validSyntax;
};
#endif
