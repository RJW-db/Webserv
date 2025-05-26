#ifndef ACONFIG_HPP
#define ACONFIG_HPP
#include <string>

using namespace std;
#include <map>
#include <vector>

class Aconfig
{
	public:
		virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes

		string error_page(string line, bool &findColon);
		string root(string line, bool &findColon);
		string ClientMaxBodysize(string line, bool &findColon);
		string indexPage(string line, bool &findColon);
		
		size_t clientBodySize;
		protected:
		Aconfig() = default;
		map<uint16_t, string> ErrorCodesWithPage;
		vector<uint16_t> ErrorCodesWithoutPage;
		string _root;
		vector<string> _indexPage;
};

#endif