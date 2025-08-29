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
        //initialization
        virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes
        Aconfig &operator=(const Aconfig &other);
        
        //parsing logic
        bool root(string &line);
        bool error_page(string &line);
        bool ClientMaxBodysize(string &line);
        bool indexPage(string &line);
        bool autoIndex(string &line);
        bool returnRedirect(string &line);

        //getters
        size_t getClientMaxBodySize() const;
        string getRoot() const;
        int8_t getAutoIndex() const;
        pair<uint16_t, string> getReturnRedirect() const;
        map<uint16_t, string> getErrorCodesWithPage() const;
        vector<string> getIndexPage() const;
        
        //set default
        void setDefaultErrorPages();

        //utils
        void setLineNbr(int num);

    protected:
        //initialization for inheriting classes
        Aconfig();
        Aconfig(const Aconfig &other);

        //stored variables
        int8_t _autoIndex;
        size_t _clientMaxBodySize;
        string _root;
        pair<uint16_t, string> _returnRedirect;
        map<uint16_t, string> ErrorCodesWithPage;
        vector<string> _indexPage;

        //util variables
        vector<uint16_t> ErrorCodesWithoutPage;
        int _lineNbr;
        
        //util functions
        bool setErrorPage(string &line, bool &foundPage);
        bool handleNearEndOfLine(string &line, size_t pos, string err);
};
#endif
