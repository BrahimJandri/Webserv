#include "ConfigParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

ConfigParser::ConfigParser()
    : filename(""), lines(), serverConfigs() {}

ConfigValue::ConfigValue()
    : stringValue(""), intValue(0), boolValue(false), type(STRING) {}

ConfigValue::ConfigValue(const std::string &val)
    : stringValue(val), intValue(0), boolValue(false), type(STRING) {}

ConfigValue::ConfigValue(int val)
    : stringValue(""), intValue(val), boolValue(false), type(INT) {}

ConfigValue::ConfigValue(bool val)
    : stringValue(""), intValue(0), boolValue(val), type(INT) {}

ConfigValue::ConfigValue(const std::vector<std::string> &val)
    : stringValue(""), intValue(0), boolValue(false), listValue(val), type(LIST) {}

ConfigValue::ConfigValue(const std::map<std::string, std::string> &val)
    : stringValue(""), intValue(0), boolValue(false), mapValue(val), type(MAP) {}

std::string ConfigValue::asString() const
{
    return stringValue;
}

int ConfigValue::asInt() const
{
    return intValue;
}

bool ConfigValue::asBool() const
{
    return boolValue;
}

std::vector<std::string> ConfigValue::asList() const
{
    return listValue;
}

std::map<std::string, std::string> ConfigValue::asMap() const
{
    return mapValue;
}

void ConfigValue::print() const
{
    switch (type)
    {
    case STRING:
        std::cout << "(" << stringValue << ")";
        break;
    case INT:
        std::cout << "{" << intValue << "}";
        break;
    case BOOL:
        std::cout << (boolValue ? "true" : "false");
        break;
    case LIST:
        for (size_t i = 0; i < listValue.size(); ++i)
        {
            if (i > 0)
                std::cout << " ";
            std::cout << "#" << listValue[i] << "#";
        }
        break;
    case MAP:
        for (std::map<std::string, std::string>::const_iterator it = mapValue.begin();
             it != mapValue.end(); ++it)
        {
            std::cout << it->first << ":" << "[" << it->second << "]" << " ";
        }
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
LocationConfig::LocationConfig() {}

LocationConfig::LocationConfig(const std::string &p) : path(p) {}

void LocationConfig::addDirective(const std::string &key, const ConfigValue &value)
{
    directives[key] = value;
}

ConfigValue LocationConfig::getDirective(const std::string &key) const
{
    std::map<std::string, ConfigValue>::const_iterator it = directives.find(key);
    if (it != directives.end())
    {
        return it->second;
    }
    return ConfigValue(); // Return empty string value
}

void LocationConfig::print() const
{
    std::cout << "    Location: " << path << std::endl;
    for (std::map<std::string, ConfigValue>::const_iterator it = directives.begin();
         it != directives.end(); ++it)
    {
        std::cout << "      " << "@" << it->first << "@: ";
        it->second.print();
        std::cout << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
ServerConfig::ServerConfig() {}

void ServerConfig::addDirective(const std::string &key, const ConfigValue &value)
{
    directives[key] = value;
}

void ServerConfig::addLocation(const LocationConfig &location)
{
    locations[location.path] = location;
}

ConfigValue ServerConfig::getDirective(const std::string &key) const
{
    std::map<std::string, ConfigValue>::const_iterator it = directives.find(key);
    if (it != directives.end())
    {
        return it->second;
    }
    return ConfigValue(); // Return empty string value
}

LocationConfig ServerConfig::getLocation(const std::string &path) const
{
    std::map<std::string, LocationConfig>::const_iterator it = locations.find(path);
    if (it != locations.end())
    {
        return it->second;
    }
    return LocationConfig(); // Return empty location
}

void ServerConfig::print() const
{
    std::cout << "  Server Directives:" << std::endl;
    for (std::map<std::string, ConfigValue>::const_iterator it = directives.begin();
         it != directives.end(); ++it)
    {
        std::cout << "    " << "@" << it->first << "@: ";
        it->second.print();
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "  Locations:" << std::endl;
    for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        it->second.print();
    }
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

ConfigParser::ConfigParser(const std::string &file) : filename(file) {}

void ConfigParser::parse()
{
    readFile();

    size_t i = 0;
    while (i < lines.size())
    {
        std::string line = lines[i];

        if (line.find("server") != std::string::npos)
        {
            ServerConfig serverConfig = parseServerBlock(i);
            serverConfigs.push_back(serverConfig);
            i = findClosingBrace(i) + 1; // Move to the next line after the closing brace
        }
        else
        {
            ++i; // Skip non-server lines
        }
    }
}

const std::vector<ServerConfig> &ConfigParser::getServerConfigs() const
{
    return serverConfigs;
}

const ServerConfig &ConfigParser::getServerConfig(size_t index) const
{
    if (index >= serverConfigs.size())
    {
        throw std::runtime_error("Server index out of bounds");
    }
    return serverConfigs[index];
}

size_t ConfigParser::getServerCount() const
{
    return serverConfigs.size();
}

void ConfigParser::printConfig() const
{
    std::cout << "=== CONFIGURATION WITH " << serverConfigs.size() << " SERVER(S) ===" << std::endl;
    for (size_t i = 0; i < serverConfigs.size(); ++i)
    {
        std::cout << "SERVER " << (i + 1) << ":" << std::endl;
        serverConfigs[i].print();
        std::cout << std::endl;
    }
}

void ConfigParser::readFile()
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        // Skip empty lines and comments
        if (!line.empty() && line[0] != '#')
        {
            lines.push_back(line);
        }
    }
    file.close();
}

ConfigValue ConfigParser::parseDirectiveValue(const std::string &key, const std::string &value)
{
    if (key == "error_page")
    {                                                            // Handle error_page directive (format: "404 500 /error.html")
        std::vector<std::string> parts = splitWhitespace(value); // Split value into parts by whitespace
        if (parts.size() >= 2)
        {                                                // Need at least error code(s) and page path
            std::map<std::string, std::string> errorMap; // Create map to store codes and page
            std::string codes = "";                      // String to concatenate all error codes
            for (size_t i = 0; i < parts.size() - 1; ++i)
            { // Iterate through all parts except the last (page path)
                if (i > 0)
                    codes += " ";  // Add space separator between codes
                codes += parts[i]; // Append current error code to codes string
            }
            errorMap["codes"] = codes;                  // Store concatenated codes in map
            errorMap["page"] = parts[parts.size() - 1]; // Store page path as last element
            return ConfigValue(errorMap);               // Return ConfigValue containing the error map
        }
    }
    else if (key == "allowed_methods")
    {                                               // Handle allowed_methods directive (format: "GET POST PUT")
        return ConfigValue(splitWhitespace(value)); // Return list of methods split by whitespace
    }
    else if (key == "port")
    { // Handle port directive (format: "8080")
        if (isNumber(value))
        {                                  // Check if value is a valid number
            std::istringstream iss(value); // Create string stream for parsing
            int port;                      // Variable to store parsed port number
            iss >> port;                   // Extract integer from string stream
            return ConfigValue(port);      // Return ConfigValue containing the port number
        }
    }
    else if (key == "limit_client_body_size")
    {                                         // Handle client body size limit (format: "10M" or "1024")
        return ConfigValue(parseSize(value)); // Parse size string (handles MB suffix) and return as int
    }
    else if (key == "autoindex" || key == "upload_enable")
    {                                                                                        // Handle boolean directives
        std::string lowerValue = value;                                                      // Create copy of value for case conversion
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower); // Convert to lowercase
        return ConfigValue(lowerValue == "on" || lowerValue == "true");                      // Return boolean based on value
    }

    return ConfigValue(value);
}

std::pair<std::string, std::string> ConfigParser::parseDirectiveLine(const std::string &line)
{
    std::string cleanLine = line;

    // Remove trailing semicolon
    if (!cleanLine.empty() && cleanLine[cleanLine.length() - 1] == ';')
    {
        cleanLine = cleanLine.substr(0, cleanLine.length() - 1);
    }

    cleanLine = trim(cleanLine);
    if (cleanLine.empty())
    {
        return std::make_pair("", "");
    }

    size_t spacePos = cleanLine.find(' ');
    if (spacePos == std::string::npos)
    {
        return std::make_pair("", "");
    }

    std::string key = cleanLine.substr(0, spacePos);
    std::string value = trim(cleanLine.substr(spacePos + 1));

    return std::make_pair(key, value);
}

ServerConfig ConfigParser::parseServerBlock(size_t startIndex)
{
    ServerConfig serverConfig;
    int endIndex = findClosingBrace(startIndex);

    size_t i = startIndex + 1; // Skip the server { line

    while (i <= static_cast<size_t>(endIndex))
    {
        std::string line = lines[i];

        if (line.find("location") != std::string::npos)
        {
            LocationConfig location = parseLocationBlock(i);
            serverConfig.addLocation(location);
            i = findClosingBrace(i) + 1;
        }
        else if (line.find("}") != std::string::npos)
        {
            break;
        }
        else
        {
            std::pair<std::string, std::string> directive = parseDirectiveLine(line);
            if (!directive.first.empty())
            {
                ConfigValue value = parseDirectiveValue(directive.first, directive.second);
                serverConfig.addDirective(directive.first, value);
            }
            ++i;
        }
    }

    return serverConfig;
}

LocationConfig ConfigParser::parseLocationBlock(size_t startIndex)
{
    std::string locationLine = lines[startIndex]; // Get the location declaration line

    // Extract location path
    size_t locationPos = locationLine.find("location"); // Find the "location" keyword in the line
    if (locationPos == std::string::npos)
    {                                                           // If "location" keyword is not found
        throw std::runtime_error("Invalid location directive"); // Throw error for invalid syntax
    }

    std::string remainder = locationLine.substr(locationPos + 8); // Extract everything after "location" (8 characters)
    remainder = trim(remainder);                                  // Remove leading/trailing whitespace from the remainder

    std::string path;                      // Variable to store the extracted path
    size_t spacePos = remainder.find(' '); // Find first space in the remainder
    size_t bracePos = remainder.find('{'); // Find opening brace in the remainder

    // Determine path based on whether space or brace comes first
    if (spacePos != std::string::npos && (bracePos == std::string::npos || spacePos < bracePos))
    {
        path = remainder.substr(0, spacePos); // Path ends at the first space
    }
    else if (bracePos != std::string::npos)
    {
        path = remainder.substr(0, bracePos); // Path ends at the opening brace
    }
    else
    {
        path = remainder; // Entire remainder is the path
    }

    path = trim(path);             // Remove any whitespace from the extracted path
    LocationConfig location(path); // Create a new LocationConfig object with the path

    int endIndex = findClosingBrace(startIndex); // Find the line index of the matching closing brace

    // Iterate through all lines within the location block
    for (int i = static_cast<int>(startIndex); i <= endIndex; ++i)
    {
        std::string line = lines[i]; // Get the current line

        // Skip location line and braces
        if (line.find("location") != std::string::npos || // Skip lines containing "location"
            line.find("{") != std::string::npos ||        // Skip lines containing opening brace
            line.find("}") != std::string::npos)
        {             // Skip lines containing closing brace
            continue; // Move to next iteration
        }

        // Parse directive
        std::pair<std::string, std::string> directive = parseDirectiveLine(line); // Parse the line into key-value pair
        if (!directive.first.empty())
        {                                                                               // If a valid directive was found
            ConfigValue value = parseDirectiveValue(directive.first, directive.second); // Convert value to appropriate type
            location.addDirective(directive.first, value);                              // Add the directive to the location config
        }
    }

    return location; // Return the populated LocationConfig object
}

int ConfigParser::findClosingBrace(size_t startIndex)
{
    int braceCount = 0;
    bool foundOpening = false;

    for (size_t i = startIndex; i < lines.size(); ++i)
    {
        std::string line = lines[i];

        for (size_t j = 0; j < line.length(); ++j)
        {
            if (line[j] == '{')
            {
                braceCount++;
                foundOpening = true;
            }
            else if (line[j] == '}')
            {
                braceCount--;
            }
        }

        if (foundOpening && braceCount == 0)
        {
            return static_cast<int>(i);
        }
    }

    throw std::runtime_error("Unclosed block found");
}

std::string trim(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        result.push_back(trim(item));
    }

    return result;
}

std::vector<std::string> splitWhitespace(const std::string &str)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (ss >> item)
    {
        result.push_back(item);
    }

    return result;
}

bool isNumber(const std::string &str)
{
    if (str.empty())
        return false;

    for (size_t i = 0; i < str.length(); ++i)
    {
        if (!std::isdigit(str[i]))
        {
            return false;
        }
    }
    return true;
}

int parseSize(const std::string &str)
{
    if (str.empty())
        return 0;

    if (str[str.length() - 1] == 'M' || str[str.length() - 1] == 'm')
    {
        std::string numberPart = str.substr(0, str.length() - 1);
        if (isNumber(numberPart))
        {
            std::istringstream iss(numberPart);
            int value;
            iss >> value;
            return value * 1024 * 1024; // Convert MB to bytes
        }
    }
    else if (isNumber(str))
    {
        std::istringstream iss(str);
        int value;
        iss >> value;
        return value;
    }

    return 0;
}
