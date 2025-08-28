#ifndef PARSING_HPP
# define PARSING_HPP

#include <ConfigServer.hpp>
#include <string>
#include <vector>
using namespace std;

class Parsing
{
	public:
    //initialization
		Parsing(const char *input);
		~Parsing() = default;
        


		
	private:
		// Helper methods for better organization
		void readConfigFile(const char *input);

        //parsing logic
        void processServerBlocks();
		template <typename T>
		    void readBlock(T &block, 
			    const map<string, bool (T::*)(string&)> &cmds, 
			    const map<string, bool (T::*)(string &)> &whileCmds);

        //checking correct syntax functions
        void ServerCheck();
        template <typename T>
            void cmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);
        template <typename T>
            void whileCmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);
        template <typename T>
            void LocationCheck(string &line, T &block);
        bool    checkParseSyntax(void);


        //line handling functions
		template <typename T>
		void skipLine(string &line, bool forceSkip, T &curConf, bool skipSpace);

        //stored variables
		vector<ConfigServer> _configs;

        //util variables
		map<int, string> _lines;
		bool _validSyntax;

    public:
        // getters
		vector<ConfigServer> &getConfigs();

        //utils
		void printAll() const;
};


#endif
