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
        size_t getClientMaxBodySize() const {
            return _clientMaxBodySize;
        }
        const string &getRoot() const {
            return _root;
        }
        int8_t getAutoIndex() const {
            return _autoIndex;
        }
        pair<uint16_t, string> getReturnRedirect() const {
            return _returnRedirect;
        }
        const map<uint16_t, string> &getErrorCodesWithPage() const {
            return _ErrorCodesWithPage;
        }
        const vector<string> &getIndexPage() const {
            return _indexPage;
        }

        // Set default
        void setDefaultErrorPages();

        // Set the current line number for error reporting
        inline void setLineNbr(int num) {
            _lineNbr = num;
        }

    protected:
        // Initialization for inheriting classes
        Aconfig();
        Aconfig(const Aconfig &other);

        // Util functions
        bool setErrorPage(string &line, bool &foundPage);
        bool handleNearEndOfLine(string &line, size_t pos, string err);
        bool setRedirect(string &line);

        int8_t _autoIndex = 0;
        size_t _clientMaxBodySize = 0;
        string _root;
        pair<uint16_t, string> _returnRedirect;
        map<uint16_t, string> _ErrorCodesWithPage;
        vector<string> _indexPage;
        vector<uint16_t> _ErrorCodesWithoutPage;
        int _lineNbr = 0;
};
#endif
