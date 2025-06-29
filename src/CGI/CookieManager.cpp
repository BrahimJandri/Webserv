// CookieManager.cpp
#include "CookieManager.hpp"

// Static method definitions (not needed if kept inline in header)

std::map<std::string, std::string> CookieManager::parseCookies(const std::string &cookieHeader)
{
    std::map<std::string, std::string> cookies;
    std::istringstream stream(cookieHeader);
    std::string token;
    while (std::getline(stream, token, ';'))
    {
        size_t pos = token.find('=');
        if (pos != std::string::npos)
        {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            cookies[trim(key)] = trim(value);
        }
    }
    return cookies;
}

std::string CookieManager::createSetCookieHeader(const std::string &key, const std::string &value, int maxAge)
{
    std::ostringstream ss;
    ss << key << "=" << value << "; Max-Age=" << maxAge << "; Path=/; HttpOnly";
    return ss.str();
}

std::string CookieManager::trim(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return str.substr(start, end - start + 1);
}
