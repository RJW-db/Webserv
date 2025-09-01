#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <sstream>

map<string, string> parseQueryString(const string& query) {
    map<string, string> result;
    istringstream stream(query);
    string pair;

    while (getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != string::npos) {
            string key = pair.substr(0, pos);
            string value = pair.substr(pos + 1);
            result[key] = value;
        }
    }
    return result;
}
#include <fcntl.h>
#include <unistd.h>
int main() {
    // Print HTTP header
    cout << "Content-Type: text/html\r\n\r\n";

    // HTML start
    cout << "<html><head><title>CGI Output</title></head><body>\n";
    cout << "<h1>C++ CGI Dynamic Page</h1>\n";

    string method = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";
    cout << "method " << method << endl; //testcout
    map<string, string> params;

    if (method == "GET") {
        string query = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
        params = parseQueryString(query);
    }
    else if (method == "POST") {
        sleep(1);
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        string body;
        body.resize(content_length);
        char buff[content_length];
        int readd = read(0, buff, 4);
        // cin.read(&body[0], content_length);
        write(1, buff, readd);
        write(1, "\n", 1);
        // params = parseQueryString(buff);
        // cout << "content length: " << content_length << endl; //testcout
    }

    if (params.empty()) {
        cout << "<p>No input received.</p>\n";
    } else {
        cout << "<h2>Received Parameters:</h2><ul>\n";
        for (map<string, string>::iterator it = params.begin(); it != params.end(); ++it) {
            cout << "<li><strong>" << it->first << "</strong>: " << it->second << "</li>\n";
        }
        cout << "</ul>\n";
    }

    // Debug info (optional)
    cout << "<h3>Environment Info:</h3><ul>";
    cout << "<li>REQUEST_METHOD: " << method << "</li>";
    cout << "<li>QUERY_STRING: " << (getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "") << "</li>";
    cout << "</ul>\n";

    // HTML end
    cout << "</body></html>\n";
    return 0;
}
