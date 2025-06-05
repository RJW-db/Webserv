#ifndef ACONFIG_HPP
#define ACONFIG_HPP
#include <string>

using namespace std;
#include <map>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <unistd.h> // access
	
enum AUTOINDEX
{
	autoIndexNotFound = -1,
	autoIndexFalse = 0, 
	autoIndexTrue = 1
};

class Aconfig
{
	public:
		virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes
		Aconfig &operator=(const Aconfig &other);
		
		bool error_page(string &line);
		bool root(string &line);
		bool ClientMaxBodysize(string &line);
		bool indexPage(string &line);
		bool autoIndex(string &line);
		bool returnRedirect(string &line);
		void setLineNbr(int num);
		void setDefaultErrorPages();

		size_t getClientBodySize() const;
		string getRoot() const;
		int8_t getAutoIndex() const;
		pair<uint16_t, string> getReturnRedirect() const;
		map<uint16_t, string> getErrorCodesWithPage() const;
		vector<string> getIndexPage() const;
		

	protected:
		size_t _clientBodySize; // for nginx could be zero but is impractical
		string _root;
		int8_t _autoIndex;
		pair<uint16_t, string> _returnRedirect;
		map<uint16_t, string> ErrorCodesWithPage;
		vector<string> _indexPage;

		Aconfig();
		Aconfig(const Aconfig &other);
		vector<uint16_t> ErrorCodesWithoutPage;
		int _lineNbr;
		bool setErrorPage(string &line, bool &foundPage);
		bool handleNearEndOfLine(string &line, size_t pos, string err);
};

#endif