// CookieManager.hpp
#ifndef COOKIE_MANAGER_HPP
#define COOKIE_MANAGER_HPP

#include <map>
#include <string>
#include <sstream>
#include <ctime>

class CookieManager
{
public:
    static std::map<std::string, std::string> parseCookies(const std::string &cookieHeader);
    static std::string createSetCookieHeader(const std::string &key, const std::string &value, int maxAge);


private:
    static std::string trim(const std::string &str);
};

#endif // COOKIE_MANAGER_HPP
