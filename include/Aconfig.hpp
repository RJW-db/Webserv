#ifndef ACONFIG_HPP
#define ACONFIG_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <map>
using namespace std;
namespace
{
    enum AUTOINDEX
    {
        autoIndexNotFound = -1,
        autoIndexFalse = 0, 
        autoIndexTrue = 1
    };
}

class Aconfig
{
    public:
        // Initialization
        virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes
        Aconfig &operator=(const Aconfig &other);
        
        // Parsing
        bool root(string &line);
        bool error_page(string &line);
        bool ClientMaxBodysize(string &line);
        bool indexPage(string &line);
        bool autoIndex(string &line);
        bool returnRedirect(string &line);

        // Getters
        size_t getClientMaxBodySize() const;
        string getRoot() const;
        int8_t getAutoIndex() const;
        pair<uint16_t, string> getReturnRedirect() const;
        map<uint16_t, string> getErrorCodesWithPage() const;
        vector<string> getIndexPage() const;
        
        // Set default
        void setDefaultErrorPages();

        // utils
        void setLineNbr(int num);

    protected:
        // Initialization for inheriting classes
        Aconfig();
        Aconfig(const Aconfig &other);

        // Util functions
        bool setErrorPage(string &line, bool &foundPage);
        bool handleNearEndOfLine(string &line, size_t pos, string err);

        int8_t _autoIndex;
        size_t _clientMaxBodySize;
        string _root;
        pair<uint16_t, string> _returnRedirect;
        map<uint16_t, string> ErrorCodesWithPage;
        vector<string> _indexPage;
        vector<uint16_t> ErrorCodesWithoutPage;
        int _lineNbr;
        

};
#endif
