#ifndef ACONFIG_HPP
#define ACONFIG_HPP
#include <string>

using namespace std;
#include <map>
#include <vector>
#include <stdexcept>
#include <stdint.h>
	

class Aconfig
{
	public:
		virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes
		Aconfig &operator=(const Aconfig &other);
		
		string error_page(string line, bool &findColon);
		string root(string line, bool &findColon);
		string ClientMaxBodysize(string line, bool &findColon);
		string indexPage(string line, bool &findColon);
		string autoIndex(string line, bool &findColon);
		string returnRedirect(string line, bool &findColon);
		
		size_t _clientBodySize; // for nginx could be zero but is impractical
		string _root;
		bool	_autoIndex; // how to check if set
		pair<uint16_t, string> _returnRedirect;
		map<uint16_t, string> ErrorCodesWithPage;
		vector<string> _indexPage;
	protected:
		Aconfig(const Aconfig &other);
		Aconfig() = default;
		vector<uint16_t> ErrorCodesWithoutPage;
};

string handleNearEndOfLine(string &line, size_t pos, bool &findColon, string err);

#endif