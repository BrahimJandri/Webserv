#include "ConfigParser.hpp"

// Utils::log("HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERE", AnsiColor::BOLD_YELLOW);

// Location struct implementation
ConfigParser::LocationConfig::LocationConfig() : autoindex(false) {}

// Listen struct implementation
ConfigParser::Listen::Listen() : host("0.0.0.0") {}

ConfigParser::Listen::Listen(const std::string &h, const std::string &p) : host(h), port(p) {}

// Server struct implementation
ConfigParser::ServerConfig::ServerConfig() : limit_client_body_size(0) {}

// ConfigParser implementation
ConfigParser::ConfigParser() : pos(0), line_number(1) {}

void ConfigParser::skipWhitespace()
{
    while (pos < content.length() && std::isspace(content[pos]))
    {
        if (content[pos] == '\n')
        {
            line_number++;
        }
        pos++;
    }
}

void ConfigParser::skipComments()
{
    while (true)
    {
        skipWhitespace();
        if (pos < content.length() && content[pos] == '#')
        {
            // Skip the entire comment line
            while (pos < content.length() && content[pos] != '\n')
            {
                pos++;
            }
            // Continue the loop to check for more comments
        }
        else
        {
            break; // No more comments, exit the loop
        }
    }
}

std::string ConfigParser::parseToken()
{
    skipComments();
    if (pos >= content.length())
    {
        return "ERROR";
    }

    std::string token;

    // Handle quoted strings
    if (content[pos] == '"' || content[pos] == '\'')
    {
        char quote = content[pos];
        pos++;
        while (pos < content.length() && content[pos] != quote)
        {
            if (content[pos] == '\\' && pos + 1 < content.length())
            {
                pos++; // Skip escape character
            }
            token += content[pos];
            pos++;
        }
        if (pos < content.length())
        {
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
           content[pos] != '#')
    {
        token += content[pos];
        pos++;
    }

    return token;
}

char ConfigParser::parseSpecialChar()
{
    skipComments();
    if (pos < content.length() &&
        (content[pos] == ';' || content[pos] == '{' || content[pos] == '}'))
    {
        return content[pos++];
    }
    return '\0';
}

std::string ConfigParser::parseDirectiveValue()
{
    std::string value;
    std::string token;
    // printPos();Rachid for debuging
    while ((token = parseToken()) != "")
    {
        // std::cout << token << std::endl; // For deubging

        // Check if this token looks like a directive keyword (this would indicate missing semicolon)
        if (value.empty() == false && (token == "server_name" || token == "listen" || token == "error_page" ||
                                       token == "limit_client_body_size" || token == "autoindex" || token == "location" ||
                                       token == "root" || token == "index" || token == "allowed_methods" || token == "cgi_map" ||
                                       token == "return"))
        {
            throw std::runtime_error("Missing semicolon after directive value '" + value + "' before '" + token + "' at line " + intToString(line_number));
        }

        if (!value.empty())
        {
            value += " ";
        }
        value += token;

        skipComments();
        if (pos < content.length() && content[pos] == ';')
        {
            break;
        }

        // Check if we reached end of block or end of file without semicolon
        if (pos >= content.length() || content[pos] == '}' || content[pos] == '{')
        {
            throw std::runtime_error("Missing semicolon after directive value '" + value + "' at line " + intToString(line_number));
        }
    }

    return value;
}

std::vector<std::string> ConfigParser::parseMultipleValues()
{
    std::vector<std::string> values;
    std::string token;

    while ((token = parseToken()) != "")
    {
        // Check if this token looks like a directive keyword (this would indicate missing semicolon)
        if (!values.empty() && (token == "server_name" || token == "listen" || token == "error_page" ||
                                token == "limit_client_body_size" || token == "autoindex" || token == "location" ||
                                token == "root" || token == "index" || token == "allowed_methods" || token == "return" || token == "cgi_map"))
        {
            throw std::runtime_error("Missing semicolon after directive values before '" + token + "' at line " + intToString(line_number));
        }

        values.push_back(token);

        skipComments();
        if (pos < content.length() && content[pos] == ';')
        {
            break;
        }

        // Check if we reached end of block or end of file without semicolon
        if (pos >= content.length() || content[pos] == '}' || content[pos] == '{')
        {
            throw std::runtime_error("Missing semicolon after directive values at line " + intToString(line_number));
        }
    }

    return values;
}

bool ConfigParser::expectSemicolon()
{
    skipComments();
    if (pos < content.length() && content[pos] == ';')
    {
        pos++; // consume the semicolon
        return true;
    }
    return false;
}

bool ConfigParser::isAtBlockBoundary()
{
    skipComments();
    return (pos < content.length() && (content[pos] == '{' || content[pos] == '}'));
}

size_t ConfigParser::parseSizeToBytes(const std::string &input)
{
    if (input.empty())
        throw std::invalid_argument("Empty size string");

    // Find where the numeric part ends
    size_t i = 0;
    while (i < input.size() && (isdigit(input[i]) || input[i] == '.'))
        ++i;

    std::string numberStr = input.substr(0, i);
    std::string unitStr = input.substr(i);

    // Convert number
    char *end = NULL;
    double number = std::strtod(numberStr.c_str(), &end);
    if (*end != '\0')
        throw std::invalid_argument("Invalid number in size: " + numberStr);

    // Normalize unit to uppercase (C++98-style loop)
    for (size_t j = 0; j < unitStr.size(); ++j)
        unitStr[j] = std::toupper(unitStr[j]);

    size_t multiplier = 1; // default: bytes

    if (unitStr == "K")
        multiplier = 1024;
    else if (unitStr == "M")
        multiplier = 1024 * 1024;
    else if (unitStr == "G")
        multiplier = 1024 * 1024 * 1024;
    else if (unitStr.empty())
        return static_cast<size_t>(number); // No unit → assume already in bytes
    else
        throw std::invalid_argument("Invalid unit in size: " + unitStr);

    return static_cast<size_t>(number * multiplier);
}

void ConfigParser::parseLocation(LocationConfig &location)
{
    if (parseSpecialChar() != '{')
    {
        throw std::runtime_error("Expected '{' after location directive at line " +
                                 intToString(line_number));
    }

    std::string directive;
    bool autoindex_seen = false;
    while (true)
    {
        // Check if we're at the end of the block before trying to parse a directive

        skipComments();
        if (pos < content.length() && content[pos] == '}')
            break;

        directive = parseToken();
        if (directive.empty())
        {
            throw std::runtime_error("Unexpected end of file in location block at line " +
                                     intToString(line_number));
        }

        if (directive == "root")
        {
            if (!location.root.empty())
                throw std::runtime_error("Duplicate 'root' directive in location block");

            std::string root_value = parseDirectiveValue();
            
            if (root_value.empty() || root_value.find(" ") != std::string::npos)
                throw std::runtime_error("'root' directive cannot be empty in location block at line " + intToString(line_number));

            location.root = root_value;
        }
        else if (directive == "index")
        {
            if(!location.index.empty())
                throw std::runtime_error("Duplicate 'index' directive in location block");
            std::string index_value = parseDirectiveValue();

            if (index_value.empty() || index_value.find(" ") != std::string::npos)
                throw std::runtime_error("'index' directive cannot be empty in location block at line " + intToString(line_number));
            location.index = index_value;
        }
        else if (directive == "allowed_methods")
        {
            if(!location.allowed_methods.empty())
                throw std::runtime_error("Duplicate 'allowed_methods' directive in location block");
            std::vector<std::string> methods = parseMultipleValues();
            if (methods.empty())
            {
                throw std::runtime_error("'allowed_methods' directive cannot be empty in location block at line " + intToString(line_number));
            }
            location.allowed_methods = methods;
        }
        else if (directive == "autoindex")
        {
            if (autoindex_seen)
                throw std::runtime_error("Duplicate 'autoindex' in the same location block");
            autoindex_seen = true;

            std::string value = parseDirectiveValue();
            if (value.empty() || (value != "off" && value != "on"))
                throw std::runtime_error("Unvalid value for autoindex: Use only 'on' or 'off'");
            location.autoindex = (value == "on");
        }
        else if (directive == "cgi_map")
        {
            std::string ext = parseToken();
            std::string interpreter = parseDirectiveValue();
            if (ext.empty() || interpreter.empty())
            {
                throw std::runtime_error("'cgi_map' directive requires both extension and interpreter at line " + intToString(line_number));
            }
            if (ext[0] != '.' || ext[1] == '.')
            {
                throw std::runtime_error("CGI extension must start with '.' at line " + intToString(line_number));
            }
            if (ext.length() < 2)
            {
                throw std::runtime_error("CGI extension must be at least two characters long at line " + intToString(line_number));
            }
            if (location.cgi.find(ext) != location.cgi.end())
            {
                throw std::runtime_error("Duplicate CGI mapping for extension '" + ext + "' at line " + intToString(line_number));
            }
            location.cgi[ext] = interpreter;
            if (access(interpreter.c_str(), X_OK) != 0)
            {
                throw std::runtime_error("CGI interpreter path is invalid or not executable: '" + interpreter +
                                         "' at line " + intToString(line_number));
            }
        }
        else if (directive == "return")
        {
            if(!location.return_directive.empty())
                throw std::runtime_error("Duplicate 'return' directive in location block");

            location.return_directive = parseDirectiveValue();
            if(location.return_directive.empty() || location.return_directive.find(" ") != std::string::npos)
                throw std::runtime_error("'return' directive cannot be empty at line " + intToString(line_number));
        }
        else
            // Skip unknown directive
            throw std::runtime_error("Unknown Directive " + intToString(line_number));

        if (!expectSemicolon())
        {
            throw std::runtime_error("Expected ';' after directive '" + directive +
                                     "' at line " + intToString(line_number));
        }
    }

    if (parseSpecialChar() != '}')
    {
        throw std::runtime_error("Expected '}' to close location block at line " +
                                 intToString(line_number));
    }
}



void ConfigParser::parseServer(ServerConfig &server)
{
    if (parseSpecialChar() != '{')
    {
        throw std::runtime_error("Expected '{' after server directive at line " +
                                 intToString(line_number));
    }

    std::string directive;
    bool bodysize_seen = false;
    while (true)
    {
        // Check if we're at the end of the block before trying to parse a directive
        skipComments();
        if (pos < content.length() && content[pos] == '}')
        {
            break;
        }

        directive = parseToken();
        if (directive.empty())
        {
            throw std::runtime_error("Unexpected end of file in server block at line " +
                                     intToString(line_number));
        }

        if (directive == "listen")
        {
            std::string listen_value = parseDirectiveValue();
            if (listen_value.empty())
            {
                throw std::runtime_error("'listen' directive cannot be empty at line " + intToString(line_number));
            }
            Listen listen_info = parseListen(listen_value);
            server.listen.push_back(listen_info);
        }
        else if (directive == "server_name")
        {
            if (!server.server_name.empty())
                throw std::runtime_error("Duplicate 'server_name' directive in server block");

            server.server_name = parseDirectiveValue();

            if (!isValidServerName(server.server_name))
                throw std::runtime_error("Unvalid 'server_name' at line " + intToString(line_number));
        }
        else if (directive == "error_page")
        {
            std::vector<std::string> values = parseMultipleValues();

            if (values.size() < 2)
            {
                throw std::runtime_error("'error_page' directive requires at least one error code and a page path at line " + intToString(line_number));
            }

            std::string error_page = values.back();

            // Validate the error_page is not a number by mistake
            char *endptr;
            std::strtol(error_page.c_str(), &endptr, 10);
            if (*endptr == '\0') // It was a number
            {
                throw std::runtime_error("Expected a file path for 'error_page' directive at line " + intToString(line_number));
            }

            for (size_t i = 0; i < values.size() - 1; ++i)
            {
                int error_code = std::atoi(values[i].c_str());

                if (error_code < 100 || error_code > 599)
                {
                    throw std::runtime_error("Invalid error code '" + values[i] + "' in 'error_page' directive at line " + intToString(line_number));
                }

                server.error_pages[error_code] = error_page;
            }
        }

        else if (directive == "limit_client_body_size")
        {
            if (bodysize_seen)
                throw std::runtime_error("Duplicate 'limit_client_body_size' at line " + intToString(line_number));
            bodysize_seen = true;

            std::string value = parseDirectiveValue();
            if (value.empty() || value == "0")
                throw std::runtime_error("'limit_client_body_size' directive cannot be empty at line " + intToString(line_number));

            try
            {
                server.limit_client_body_size = parseSizeToBytes(value);
            }
            catch (const std::invalid_argument &e)
            {
                throw std::runtime_error("Invalid value for 'limit_client_body_size': " + value + " at line " + intToString(line_number));
            }
        }
        else if (directive == "root")
        {
            if (!server.root.empty())
                throw std::runtime_error("Duplicate 'root' directive in server block");

            std::string root_value = parseDirectiveValue();
            if (root_value.empty())
            {
                throw std::runtime_error("'root' directive cannot be empty in server block at line " + intToString(line_number));
            }
            server.root = root_value;
        }
        else if (directive == "location")
        {
            LocationConfig location;
            location.path = parseToken();
            parseLocation(location);
            server.locations.push_back(location);
            continue; // Skip semicolon check for location block
        }
        else
        {
            // Skip unknown
            throw std::runtime_error("Unknown Directive " + intToString(line_number));
            parseDirectiveValue();
        }

        if (!expectSemicolon())
        {
            throw std::runtime_error("Expected ';' after directive '" + directive +
                                     "' at line " + intToString(line_number));
        }
    }
    if (server.root.empty())
    {
        throw std::runtime_error("Missing required 'root' directive in server block at line " +
                                 intToString(line_number));
    }

    if (parseSpecialChar() != '}')
    {
        throw std::runtime_error("Expected '}' to close server block at line " +
                                 intToString(line_number));
    }
}

bool ConfigParser::isValidServerName(const std::string &name)
{
    if (name.empty())
        throw std::runtime_error("'server_name' directive cannot be empty at line " + intToString(line_number));

    if (name.find(" ") != std::string::npos)
        return false;
    return true;
}

ConfigParser::Listen ConfigParser::parseListen(const std::string &listen_value)
{
    size_t colon_pos = listen_value.find(':');

    if (colon_pos != std::string::npos)
    {
        // Format: host:port
        std::string host = listen_value.substr(0, colon_pos);
        std::string port = listen_value.substr(colon_pos + 1);

        if (host.empty())
            throw std::runtime_error("Invalid listen value, missed host before colon");

        if (!isValidPort(port))
            throw std::runtime_error("Invalid Port number in listen value: " + port);

        if (host == "localhost")
            return Listen("0.0.0.0", port);

        if (!isValidIPv4(host))
            throw std::runtime_error("Invalid IP address format in listen value: " + host);

        return Listen(host, port);
    }
    else
    {
        if (!isValidPort(listen_value))
            throw std::runtime_error("Invalid Port number in listen value: " + listen_value);

        return Listen("0.0.0.0", listen_value);
    }
}

bool ConfigParser::isValidPort(const std::string &port)
{
    if (!isDigitString(port))
        return false;

    int num = atoi(port.c_str());
    if (num < 1 || num > 65535)
        return false;

    return true;
}

bool ConfigParser::isValidIPv4(const std::string &ip)
{
    size_t start = 0;
    int bytes = 0;

    while (start < ip.size() && bytes < 4)
    {
        size_t end = ip.find(".", start);

        if (end == std::string::npos)
            end = ip.size();
        std::string byte = ip.substr(start, end - start);

        if (byte.empty())
            return false;

        if (!isDigitString(byte))
            return false;

        int num = atoi(byte.c_str());
        if (num < 0 || num > 255)
            return false;

        bytes += 1;

        start = end + 1;
    }
    return (bytes == 4 && start > ip.size());
}

bool ConfigParser::isDigitString(const std::string &str)
{

    for (size_t i = 0; i < str.size(); i++)
    {
        if (!isdigit(str[i]))
            return false;
    }
    return true;
}

std::string ConfigParser::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void ConfigParser::validatePorts()
{
    std::set<std::string> global_ports; // track all host:port across all servers

    for (size_t i = 0; i < servers.size(); i++)
    {
        std::set<std::string> server_ports; // track ports inside this server

        for (size_t j = 0; j < servers[i].listen.size(); j++)
        {
            const Listen &listen_info = servers[i].listen[j];
            std::string port_key = listen_info.host + ":" + listen_info.port;

            // Check duplicates inside the same server
            if (server_ports.find(port_key) != server_ports.end())
            {
                throw std::runtime_error("Duplicate port found in the same server: " + port_key);
            }
            server_ports.insert(port_key);

            // Check duplicates across all servers (regardless of server_name)
            if (global_ports.find(port_key) != global_ports.end())
            {
                throw std::runtime_error("Duplicate port found across different servers: " + port_key);
            }
            global_ports.insert(port_key);
        }
    }
}

void ConfigParser::validateRequiredDirectives()
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        const ServerConfig &server = servers[i];

        // Check if server has at least one listen directive and limit_client_body_size
        if (server.listen.empty() || server.limit_client_body_size == 0)
        {
            throw std::runtime_error("Server block missing required directive");
        }

        // Only check if server has root directive - don't worry about locations
        if (server.root.empty())
        {
            throw std::runtime_error("Server block missing required 'root' directive");
        }
    }
}


void ConfigParser::parseFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    file.close();

    if (content.empty())
    {
        throw std::runtime_error("Configuration file is empty: " + filename);
    }

    parse();
}

void ConfigParser::parseString(const std::string &config_content)
{
    content = config_content;
    pos = 0;
    line_number = 1;
    servers.clear();
    parse();
}

void ConfigParser::parse()
{
    std::string directive;
    while ((directive = parseToken()) != "ERROR")
    {
        if (directive == "server")
        {
            ServerConfig server;
            parseServer(server);
            servers.push_back(server);
        }
        else
        {
            throw std::runtime_error("Unknown directive: " + directive +
                                     " at line " + intToString(line_number));
        }
    }
    validatePorts();
    validateRequiredDirectives();
}

size_t ConfigParser::getServerCount() const
{
    return servers.size();
}

void ConfigParser::printConfig()
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << AnsiColor::BOLD_BLUE << "Server " << i + 1 << ":" << AnsiColor::BOLD_MAGENTA << std::endl;

        std::cout << AnsiColor::BOLD_CYAN << "  Server name: " << AnsiColor::BOLD_MAGENTA << servers[i].server_name << std::endl;

        std::cout << AnsiColor::BOLD_CYAN << "  Listen: " << AnsiColor::BOLD_GREEN;
        for (size_t j = 0; j < servers[i].listen.size(); j++)
        {
            const Listen &listen_info = servers[i].listen[j];
            std::cout << AnsiColor::BOLD_GREEN
                      << "http://" << listen_info.host << ":" << listen_info.port
                      << AnsiColor::RESET;
            if (j < servers[i].listen.size() - 1)
                std::cout << ", ";
        }
        std::cout << std::endl;
    }
}

const std::vector<ConfigParser::ServerConfig> &ConfigParser::getServers() const
{
    return servers;
}

size_t ConfigParser::getListenCount() const
{
    size_t ListenCount = 0;
    for (size_t i = 0; i < servers.size(); i++)
    {
        ListenCount += servers[i].listen.size();
    }
    return ListenCount;
}

void ConfigParser::printPos()
{

    std::cout << pos << "---> ";
    std::cout << line_number << std::endl;
}