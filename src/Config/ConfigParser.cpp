#include "ConfigParser.hpp"


// Location struct implementation
ConfigParser::Location::Location() : autoindex(false) {}

// Listen struct implementation
ConfigParser::Listen::Listen() : host("0.0.0.0") {}

ConfigParser::Listen::Listen(const std::string& h, const std::string& p) : host(h), port(p) {}

// Server struct implementation
ConfigParser::Server::Server() : autoindex(false) {}

// ConfigParser implementation
ConfigParser::ConfigParser() : pos(0), line_number(1) {}

void ConfigParser::skipWhitespace() {
    while (pos < content.length() && std::isspace(content[pos])) {
        if (content[pos] == '\n') {
            line_number++;
        }
        pos++;
    }
}

void ConfigParser::skipComments() {
    skipWhitespace();
    if (pos < content.length() && content[pos] == '#') {
        while (pos < content.length() && content[pos] != '\n') {
            pos++;
        }
        skipWhitespace();
    }
}

std::string ConfigParser::parseToken() {
    skipComments();
    if (pos >= content.length()) {
        return "";
    }
    
    std::string token;
    
    // Handle quoted strings
    if (content[pos] == '"' || content[pos] == '\'') {
        char quote = content[pos];
        pos++;
        while (pos < content.length() && content[pos] != quote) {
            if (content[pos] == '\\' && pos + 1 < content.length()) {
                pos++; // Skip escape character
            }
            token += content[pos];
            pos++;
        }
        if (pos < content.length()) {
            pos++; // Skip closing quote
        }
        return token;
    }
    
    // Handle regular tokens
    while (pos < content.length() && 
           !std::isspace(content[pos]) && 
           content[pos] != ';' && 
           content[pos] != '{' && 
           content[pos] != '}' &&
           content[pos] != '#') {
        token += content[pos];
        pos++;
    }
    
    return token;
}

char ConfigParser::parseSpecialChar() {
    skipComments();
    if (pos < content.length() && 
        (content[pos] == ';' || content[pos] == '{' || content[pos] == '}')) {
        return content[pos++];
    }
    return '\0';
}

std::string ConfigParser::parseDirectiveValue() {
    std::string value;
    std::string token;
    
    while ((token = parseToken()) != "") {
        if (!value.empty()) {
            value += " ";
        }
        value += token;
        
        skipComments();
        if (pos < content.length() && content[pos] == ';') {
            break;
        }
    }
    
    return value;
}

std::vector<std::string> ConfigParser::parseMultipleValues() {
    std::vector<std::string> values;
    std::string token;
    
    while ((token = parseToken()) != "") {
        values.push_back(token);
        
        skipComments();
        if (pos < content.length() && content[pos] == ';') {
            break;
        }
    }
    
    return values;
}

bool ConfigParser::expectSemicolon() {
    skipComments();
    if (pos < content.length() && content[pos] == ';') {
        pos++; // consume the semicolon
        return true;
    }
    return false;
}

bool ConfigParser::isAtBlockBoundary() {
    skipComments();
    return (pos < content.length() && (content[pos] == '{' || content[pos] == '}'));
}

void ConfigParser::parseLocation(Location& location) {
    if (parseSpecialChar() != '{') {
        throw std::runtime_error("Expected '{' after location directive at line " + 
                               intToString(line_number));
    }
    
    std::string directive;
    while (true) {
        // Check if we're at the end of the block before trying to parse a directive
        skipComments();
        if (pos < content.length() && content[pos] == '}') {
            break;
        }
        
        directive = parseToken();
        if (directive.empty()) {
            throw std::runtime_error("Unexpected end of file in location block at line " + 
                                   intToString(line_number));
        }
        
        if (directive == "root") {
            location.root = parseDirectiveValue();
        } else if (directive == "index") {
            location.index = parseMultipleValues();
        } else if (directive == "allowed_methods") {
            location.allowed_methods = parseMultipleValues();
        } else if (directive == "autoindex") {
            std::string value = parseDirectiveValue();
            location.autoindex = (value == "on");
        } else if (directive == "cgi_extension") {
            location.cgi_extension = parseDirectiveValue();
        } else if (directive == "cgi_path") {
            location.cgi_path = parseDirectiveValue();
        } else if (directive == "return") {
            location.return_directive = parseDirectiveValue();
        } else {
            // Skip unknown directive
            parseDirectiveValue();
        }
        
        if (!expectSemicolon()) {
            throw std::runtime_error("Expected ';' after directive '" + directive + 
                                   "' at line " + intToString(line_number));
        }
    }
    
    if (parseSpecialChar() != '}') {
        throw std::runtime_error("Expected '}' to close location block at line " + 
                               intToString(line_number));
    }
}

void ConfigParser::parseServer(Server& server) {
    if (parseSpecialChar() != '{') {
        throw std::runtime_error("Expected '{' after server directive at line " + 
                               intToString(line_number));
    }
    
    std::string directive;
    while (true) {
        // Check if we're at the end of the block before trying to parse a directive
        skipComments();
        if (pos < content.length() && content[pos] == '}') {
            break;
        }
        
        directive = parseToken();
        if (directive.empty()) {
            throw std::runtime_error("Unexpected end of file in server block at line " + 
                                   intToString(line_number));
        }
        
        if (directive == "listen") {
            std::string listen_value = parseDirectiveValue();
            Listen listen_info = parseListen(listen_value);
            server.listen.push_back(listen_info);
        } else if (directive == "server_name") {
            server.server_name = parseDirectiveValue();
        } else if (directive == "error_page") {
            std::vector<std::string> values = parseMultipleValues();
            if (values.size() >= 2) {
                std::string error_page = values.back();
                for (size_t i = 0; i < values.size() - 1; i++) {
                    server.error_pages[values[i]] = error_page;
                }
            }
        } else if (directive == "limit_client_body_size") {
            server.limit_client_body_size = parseDirectiveValue();
        } else if (directive == "autoindex") {
            std::string value = parseDirectiveValue();
            server.autoindex = (value == "on");
        } else if (directive == "location") {
            Location location;
            location.path = parseToken();
            parseLocation(location);
            server.locations.push_back(location);
            continue; // Skip semicolon check for location block
        } else {
            // Skip unknown directive
            parseDirectiveValue();
        }
        
        if (!expectSemicolon()) {
            throw std::runtime_error("Expected ';' after directive '" + directive + 
                                   "' at line " + intToString(line_number));
        }
    }
    
    if (parseSpecialChar() != '}') {
        throw std::runtime_error("Expected '}' to close server block at line " + 
                               intToString(line_number));
    }
}

ConfigParser::Listen ConfigParser::parseListen(const std::string& listen_value) {
    size_t colon_pos = listen_value.find(':');
    
    if (colon_pos != std::string::npos) {
        // Format: host:port
        std::string host = listen_value.substr(0, colon_pos);
        std::string port = listen_value.substr(colon_pos + 1);
        return Listen(host, port);
    } else {
        // Format: port only, use default host
        return Listen("0.0.0.0", listen_value);
    }
}

std::string ConfigParser::intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void ConfigParser::validatePorts() {
    std::set<std::string> used_ports;
    
    for (size_t i = 0; i < servers.size(); i++) {
        for (size_t j = 0; j < servers[i].listen.size(); j++) {
            const Listen& listen_info = servers[i].listen[j];
            std::string port_key = listen_info.host + ":" + listen_info.port;
            
            if (used_ports.find(port_key) != used_ports.end()) {
                throw std::runtime_error("Duplicate host:port found: " + port_key);
            }
            used_ports.insert(port_key);
        }
    }
}

void ConfigParser::parseFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    file.close();
    
    parse();
}

void ConfigParser::parseString(const std::string& config_content) {
    content = config_content;
    pos = 0;
    line_number = 1;
    servers.clear();
    parse();
}

void ConfigParser::parse() {
    std::string directive;
    while ((directive = parseToken()) != "") {
        if (directive == "server") {
            Server server;
            parseServer(server);
            servers.push_back(server);
        } else {
            throw std::runtime_error("Unknown directive: " + directive + 
                                   " at line " + intToString(line_number));
        }
    }
    
    validatePorts();
}

void ConfigParser::printConfig() {
    for (size_t i = 0; i < servers.size(); i++) {
        std::cout << "Server " << i + 1 << ":" << std::endl;
        std::cout << "  Server name: " << servers[i].server_name << std::endl;
        std::cout << "  Listen: ";
        for (size_t j = 0; j < servers[i].listen.size(); j++) {
            const Listen& listen_info = servers[i].listen[j];
            std::cout << "Host: " << listen_info.host << " Port: " << listen_info.port;
            if (j < servers[i].listen.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        std::cout << "  Limit client body size: " << servers[i].limit_client_body_size << std::endl;
        std::cout << "  Autoindex: " << (servers[i].autoindex ? "on" : "off") << std::endl;
        
        std::cout << "  Error pages:" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = servers[i].error_pages.begin(); 
             it != servers[i].error_pages.end(); ++it) {
            std::cout << "    " << it->first << " -> " << it->second << std::endl;
        }
        
        std::cout << "  Locations:" << std::endl;
        for (size_t j = 0; j < servers[i].locations.size(); j++) {
            const Location& loc = servers[i].locations[j];
            std::cout << "    Path: " << loc.path << std::endl;
            std::cout << "      Root: " << loc.root << std::endl;
            std::cout << "      Index: ";
            for (size_t k = 0; k < loc.index.size(); k++) {
                std::cout << loc.index[k];
                if (k < loc.index.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
            std::cout << "      Allowed methods: ";
            for (size_t k = 0; k < loc.allowed_methods.size(); k++) {
                std::cout << loc.allowed_methods[k];
                if (k < loc.allowed_methods.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
            std::cout << "      CGI extension: " << loc.cgi_extension << std::endl;
            std::cout << "      CGI path: " << loc.cgi_path << std::endl;
            std::cout << "      Return: " << loc.return_directive << std::endl;
            std::cout << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
        }
        std::cout << std::endl;
    }
}

const std::vector<ConfigParser::Server>& ConfigParser::getServers() const {
    return servers;
}