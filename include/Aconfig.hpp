#ifndef ACONFIG_HPP
#define ACONFIG_HPP
#include <string>

using namespace std;
#include <map>
#include <vector>
#include <stdexcept>
#include <cstdint>
	
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
		
		string error_page(string line, bool &findColon);
		bool root(string &line);
		bool ClientMaxBodysize(string &line);
		string indexPage(string line, bool &findColon);
		bool autoIndex(string &line);
		string returnRedirect(string line, bool &findColon);
		
		size_t _clientBodySize; // for nginx could be zero but is impractical
		string _root;
		int8_t _autoIndex;
		pair<uint16_t, string> _returnRedirect;
		map<uint16_t, string> ErrorCodesWithPage;
		vector<string> _indexPage;
	protected:
		Aconfig();
		Aconfig(const Aconfig &other);
		vector<uint16_t> ErrorCodesWithoutPage;
};

bool handleNearEndOfLine(string &line, size_t pos, string err);

#endif