#include "Server.hpp"

Server::Server()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
}

Server::~Server()
{
    close(epoll_fd);
    for (std::map<int, Client *>::iterator iter = clients.begin(); iter != clients.end(); ++iter)
    {
        delete iter->second;
    }
}

int create_server_socket(const std::string &host, int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt SO_REUSEADDR");
        close(server_fd);
        return -1;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);


    if (host == "0.0.0.0")
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        struct addrinfo hints;
        struct addrinfo *result;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int ret = getaddrinfo(host.c_str(), NULL, &hints, &result);
        if (ret != 0)
        {
            std::cerr << "getaddrinfo: " << gai_strerror(ret);
            close(server_fd);
            return -1;
        }
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)result->ai_addr;
        addr.sin_addr = ipv4->sin_addr;

        freeaddrinfo(result);
    }

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void Server::setupServers(const ConfigParser &parser)
{
    const std::vector<ConfigParser::ServerConfig> &serverConfigs = parser.getServers();


    for (size_t i = 0; i < serverConfigs.size(); ++i)
    {
        const ConfigParser::ServerConfig &server = serverConfigs[i];

        for (size_t j = 0; j < server.listen.size(); ++j)
        {
            const ConfigParser::Listen &listen = server.listen[j];
            std::string host = listen.host;
            std::string port = listen.port;
            std::string hostPortKey = host + ":" + port;

            Utils::log("Setting up server on " + host + ":" + port, AnsiColor::BOLD_YELLOW);

            int server_fd = create_server_socket(host, std::atoi(port.c_str()));
            if (server_fd == -1)
            {
                Utils::log("Failed to create server socket on " + host + ":" + port, AnsiColor::BOLD_RED);
                this->Cleanup();
                exit(EXIT_FAILURE);
            }

            std::vector<ConfigParser::ServerConfig> matchedConfigs;
            for (size_t k = 0; k < serverConfigs.size(); ++k)
            {
                const ConfigParser::ServerConfig &other = serverConfigs[k];
                for (size_t l = 0; l < other.listen.size(); ++l)
                {
                    if (other.listen[l].host == host && other.listen[l].port == port)
                    {
                        matchedConfigs.push_back(other);
                        break;
                    }
                }
            }

            serverConfigMap[server_fd] = matchedConfigs;

            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = server_fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
            {
                perror("epoll_ctl: server_fd");
                exit(EXIT_FAILURE);
            }
            server_fds.insert(server_fd);
        }
    }
}

volatile sig_atomic_t Server::_turnoff = 0;

void sighandler(int signum)
{
    if (signum == SIGINT || signum == SIGQUIT)
    {
        Server::_turnoff = 1;
    }
}

void Server::acceptNewConnection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        perror("accept");
        return;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        close(client_fd);
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
    {
        perror("epoll_ctl: client_fd");
        close(client_fd);
        return;
    }

    clients[client_fd] = new Client(client_fd);
    clientToServergMap[client_fd] = serverConfigMap[server_fd][0];

    Utils::log("New connection from " + std::string(inet_ntoa(client_addr.sin_addr)) + ":" + to_string_c98(ntohs(client_addr.sin_port)) + " (fd: " + to_string_c98(client_fd) + ")", AnsiColor::BOLD_GREEN);
}

void Server::closeClientConnection(int client_fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1)
    {
        perror("epoll_ctl: client_fd");
        return;
    }

    std::map<int, Client *>::iterator it = clients.find(client_fd);
    if (it != clients.end())
    {
        delete it->second;
        clients.erase(it);
    }

    if (close(client_fd) == -1)
        perror("close");

    Utils::log("Closed connection for client fd: " + to_string_c98(client_fd), AnsiColor::BOLD_RED);
}

bool isHexDigit(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

char hexToChar(char high, char low)
{
    if (!isHexDigit(high) || !isHexDigit(low))
        throw std::invalid_argument("Invalid hex digit");
    int highVal = std::isdigit(high) ? high - '0' : std::tolower(high) - 'a' + 10;
    int lowVal = std::isdigit(low) ? low - '0' : std::tolower(low) - 'a' + 10;
    return static_cast<char>((highVal << 4) | lowVal);
}

std::string removePercentEncoded(const std::string &input)
{
    std::string result;
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (input[i] == '%' && i + 2 < input.length() &&
            isHexDigit(input[i + 1]) && isHexDigit(input[i + 2]))
        {
            char decodedChar = hexToChar(input[i + 1], input[i + 2]);
            result += decodedChar;
            i += 2;
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

void normalize_path(std::string &path)
{
    if (path.empty())
        return;

    path = removePercentEncoded(path);

    bool has_leading_slash = (path[0] == '/');

    std::vector<std::string> components;
    std::string token;
    std::istringstream iss(path);
    while (std::getline(iss, token, '/'))
    {
        if (!token.empty() && token != ".")
            components.push_back(token);
    }

    path.clear();
    for (size_t i = 0; i < components.size(); ++i)
    {
        if (i > 0)
            path += "/";
        path += components[i];
    }

    if (has_leading_slash)
        path = "/" + path;

    if (path.length() > 1 && path[path.length() - 1] == '/')
        path.erase(path.length() - 1);
}

const ConfigParser::LocationConfig *findMatchingLocation(const std::vector<ConfigParser::LocationConfig> &locations, const std::string &requestPath)
{
    size_t longestMatch = 0;
    const ConfigParser::LocationConfig *bestMatch = NULL;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locPath = locations[i].path;
        if (requestPath.find(locPath) == 0 && locPath.length() > longestMatch)
        {
            longestMatch = locPath.length();
            bestMatch = &locations[i];
        }
    }

    return bestMatch;
}

bool isDirectory(const std::string &path)
{
    struct stat statbuf;
    return stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}

std::string get_file_extension(const std::string &filename)
{
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos)
        return "";
    return filename.substr(dot);
}

std::string escape_html(const std::string &input)
{
    std::ostringstream escaped;
    for (size_t i = 0; i < input.size(); ++i)
    {
        switch (input[i])
        {
        case '&':
            escaped << "&amp;";
            break;
        case '<':
            escaped << "&lt;";
            break;
        case '>':
            escaped << "&gt;";
            break;
        case '"':
            escaped << "&quot;";
            break;
        case '\'':
            escaped << "&#39;";
            break;
        default:
            escaped << input[i];
            break;
        }
    }
    return escaped.str();
}

std::string generate_autoindex(const std::string &dir_path, const std::string &uri)
{
    DIR *dir = opendir(dir_path.c_str());
    if (!dir)
        return "<html><head><meta charset=\"UTF-8\"><title>403 Forbidden</title></head><body><h1>403 Forbidden</h1></body></html>";

    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n";
    oss << "<html lang=\"ar\">\n"; // Arabic language
    oss << "<head>\n";
    oss << "  <meta charset=\"UTF-8\">\n";
    oss << "  <title>Index of " << escape_html(uri) << "</title>\n";
    oss << "  <style>body { font-family: sans-serif; direction: rtl; }</style>\n";
    oss << "</head>\n";
    oss << "<body>\n";
    oss << "  <h1>فهرس " << escape_html(uri) << "</h1>\n";
    oss << "  <hr><pre>\n";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;

        std::string full_path = dir_path + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
                name += "/";
        }

        std::string encoded_name = escape_html(name);
        oss << "<a href=\"" << escape_html(uri + (uri[uri.length() - 1] == '/' ? "" : "/") + name) << "\">"
            << encoded_name << "</a>\n";
    }

    oss << "  </pre><hr>\n";
    oss << "</body></html>\n";
    closedir(dir);
    return oss.str();
}


int Server::prepareResponse(const requestParser &req, int client_fd)
{
    ConfigParser::ServerConfig serverConfig = clientToServergMap[client_fd];
    std::string path = req.getPath();
    std::string method = req.getMethod();
    std::string version = req.getHttpVersion();

    if(path.empty() || version.empty())
    {
        send_error_response(client_fd, 400, "Bad Request", serverConfig);
        return -1;
    }
    if(version != "HTTP/1.1" && version != "HTTP/1.0")
    {
        send_error_response(client_fd, 505, "HTTP Version Not Supported", serverConfig);
        return -1;
    }
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos)
    path = path.substr(0, queryPos);
    
    
    const ConfigParser::LocationConfig *location = findMatchingLocation(serverConfig.locations, path);

    if (location && !location->return_directive.empty())
    {
        const std::string &location_url = location->return_directive;
        send_redirect_response(client_fd, 302, location_url, "Found");
        return -1;
    }

    if (location && !location->allowed_methods.empty())
    {
        if (std::find(location->allowed_methods.begin(), location->allowed_methods.end(), method) == location->allowed_methods.end())
        {
            send_error_response(client_fd, 405, "Method Not Allowed", serverConfig);
            return -1;
        }
    }

    std::string root = serverConfig.root;
    std::string full_path;
    full_path = root + path;
    normalize_path(full_path);

    size_t max_body_size = serverConfig.limit_client_body_size;

    if (!req.getBody().empty() && req.getBody().size() > max_body_size)
    {
        send_error_response(client_fd, 413, "Payload Too Large", serverConfig);
        return -1;
    }

    if (location && !location->cgi.empty())
    {
        std::string file_ext = get_file_extension(full_path);

        std::map<std::string, std::string>::const_iterator it = location->cgi.find(file_ext);
        if (it != location->cgi.end())
        {
            const std::string &interpreter = it->second;

            CGIHandler cgiHandler;
            std::string cgi_response = cgiHandler.handleCGI(full_path, req, interpreter, client_fd);

            if (cgi_response.empty())
            {
                send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
                return -1;
            }

            clients[client_fd]->setResponse(cgi_response);
            Utils::log("CGI executed for " + full_path + " method: " + method, AnsiColor::BOLD_YELLOW);
            return 0;
        }
    }

    if (access(full_path.c_str(), F_OK) == -1)
    {
        send_error_response(client_fd, 404, "Not Found", serverConfig);
        return -1;
    }

    Response response;
    bool autoindex = location ? location->autoindex : false;
    if (method == "GET")
    {
        response = Response::buildGetResponse(req, full_path, autoindex, client_fd, serverConfig);
    }
    else if (method == "POST")
    {
        response = Response::buildPostResponse(req, full_path, client_fd, serverConfig);
    }
    else if (method == "DELETE")
    {
        response = Response::buildDeleteResponse(req, full_path, client_fd, serverConfig);
    }
    else
    {
        send_error_response(client_fd, 400, "Bad Request", serverConfig);
        return -1;
    }

    std::string response_str = response.toString();
    Utils::log("Method: " + req.getMethod() + ", Path: " + req.getPath() + ", Status Code: " + to_string_c98(response.getStatusCode()), AnsiColor::BOLD_YELLOW);
    clients[client_fd]->setResponse(response_str);
    return 0;
}

void Server::handleClientRead(int client_fd)
{
    if (clients.find(client_fd) == clients.end())
    {
        closeClientConnection(client_fd);
        return;
    }

    char buffer[4096];
    ssize_t bytes_read;

    while (true)
    {
        bytes_read = read(client_fd, buffer, sizeof(buffer));

        if (bytes_read > 0)
        {
            clients[client_fd]->appendToBuffer(std::string(buffer, bytes_read));
        }
        else if (bytes_read == 0)
        {
            closeClientConnection(client_fd);
            return;
        }
        else
        {
            break;
        }
    }

    if (clients[client_fd]->processRequest())
    {
        const requestParser &request = clients[client_fd]->getRequest();

        if (prepareResponse(request, client_fd) == -1)
        {
            closeClientConnection(client_fd);
            return;
        }

        struct epoll_event event;
        event.events = EPOLLOUT | EPOLLET;
        event.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
    }
}

void Server::handleClientWrite(int client_fd)
{
    if (clients.find(client_fd) == clients.end())
    {
        closeClientConnection(client_fd);
        return;
    }

    Client *client = clients[client_fd];
    const std::string &response = client->getResponse();

    write(client_fd, response.c_str(), response.size());
    Utils::log("Sending response to client fd: " + to_string_c98(client_fd), AnsiColor::BOLD_BLUE);
    client->clearResponse();
    closeClientConnection(client_fd);
}

void Server::handleConnections()
{
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);

    struct epoll_event events[MAX_EVENTS];

    while (!_turnoff)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < num_events; ++i)
        {
            int fd = events[i].data.fd;

            if (server_fds.find(fd) != server_fds.end())
            {
                acceptNewConnection(fd);
            }
            else if (events[i].events & EPOLLIN)
            {
                handleClientRead(fd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                handleClientWrite(fd);
            }
        }
    }
}

void Server::Cleanup()
{
    if (!clients.empty())
    {
        for (std::map<int, Client *>::iterator iter = clients.begin(); iter != clients.end(); ++iter)
        {
            if (iter->first >= 0)
                close(iter->first);

            if (iter->second != NULL)
                delete iter->second;
        }
        clients.clear();
    }
    if (epoll_fd >= 0 && epoll_fd != -1)
    {
        close(epoll_fd);
    }

    if (!server_fds.empty())
    {
        for (std::set<int>::iterator it = server_fds.begin(); it != server_fds.end(); ++it)
        {
            if (*it >= 0)
                close(*it);
        }
        server_fds.clear();
    }

    if (!serverConfigMap.empty())
        serverConfigMap.clear();

    if (!clientToServergMap.empty())
        clientToServergMap.clear();
}

void send_http_headers(int client_fd, const std::string &status_line,
                       const std::string &content_type, size_t content_length,
                       const std::string &connection_header = "close")
{
    std::ostringstream oss;
    oss << status_line << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << content_length << "\r\n"
        << "Connection: " << connection_header << "\r\n\r\n";

    std::string header_response = oss.str();
    write(client_fd, header_response.c_str(), header_response.length());
}

std::string to_string_c98(size_t val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

void send_error_response(int client_fd, int status_code, const std::string &message, const ConfigParser::ServerConfig &serverConfig)
{

    std::string status_text;
    std::string error_page_path;

    switch (status_code)
    {
    case 302:
        status_text = "Moved Temporarily";
        break;
    case 400:
        status_text = "Bad Request";
        break;
    case 403:
        status_text = "Forbidden";
        break;
    case 404:
        status_text = "Not Found";
        break;
    case 405:
        status_text = "Method Not Allowed";
        break;
    case 413:
        status_text = "Payload Too Large";
        break;
    case 500:
        status_text = "Internal Server Error";
        break;
    case 504:
        status_text = "Gateway Timeout";
        break;
    case 505:
        status_text = "HTTP Version Not Supported";
        break;
    default:
        status_text = "Error";
        break;
    }

    std::map<int, std::string>::const_iterator it = serverConfig.error_pages.find(status_code);
    if (it != serverConfig.error_pages.end())
    {
        error_page_path = serverConfig.root + it->second;
    }
    else
    {
        if (status_code == 404)
            error_page_path = "./www/epages/404.html";
        else if (status_code == 500)
            error_page_path = "./www/epages/500.html";
        else if (status_code == 403)
            error_page_path = "./www/epages/403.html";
        else if (status_code == 400)
            error_page_path = "./www/epages/400.html";
        else if (status_code == 405)
            error_page_path = "./www/epages/405.html";
        else if (status_code == 413)
            error_page_path = "./www/epages/413.html";
        else if (status_code == 302)
            error_page_path = "./www/epages/302.html";
        else if (status_code == 504)
            error_page_path = "./www/epages/504.html";
        else if (status_code == 505)
            error_page_path = "./www/epages/505.html";
        else
            error_page_path = "./www/epages/500.html";
    }

    std::string response_body = "";

    int error_fd = open(error_page_path.c_str(), O_RDONLY);
    if (error_fd >= 0)
    {
        struct stat st;
        if (fstat(error_fd, &st) == 0 && st.st_size > 0)
        {
            response_body.resize(st.st_size);
            ssize_t bytesRead = read(error_fd, &response_body[0], st.st_size);
            if (bytesRead <= 0 || response_body.find_first_not_of(" \t\n\r") == std::string::npos)
            {
                response_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) +
                                "</title></head><body><h1>Error " + to_string_c98(status_code) + ": " +
                                status_text + "</h1><p>" + message + "</p></body></html>";
            }
        }
        else
        {
            response_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) +
                            "</title></head><body><h1>Error " + to_string_c98(status_code) + ": " +
                            status_text + "</h1><p>" + message + "</p></body></html>";
        }
        close(error_fd);
    }
    else
    {
        response_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) +
                        "</title></head><body><h1>Error " + to_string_c98(status_code) + ": " +
                        status_text + "</h1><p>" + message + "</p></body></html>";
    }
    send_http_headers(client_fd, "HTTP/1.1 " + to_string_c98(status_code) + " " + status_text,
                      "text/html", response_body.length());
    write(client_fd, response_body.c_str(), response_body.length());
}

void send_redirect_response(int client_fd, int status_code, const std::string &location_url, const std::string &status_text)
{
    std::string response_body = "<!DOCTYPE html><html><head><title>" + to_string_c98(status_code) + " " + status_text +
                                "</title></head><body><h1>" + status_text + "</h1><p>The document has moved <a href=\"" +
                                location_url + "\">here</a>.</p></body></html>";

    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n"
        << "Location: " << location_url << "\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << response_body.length() << "\r\n"
        << "Connection: close\r\n\r\n"
        << response_body;

    std::string response = oss.str();
    write(client_fd, response.c_str(), response.length());

    Utils::log("Sent " + to_string_c98(status_code) + " redirect to: " + location_url, AnsiColor::BOLD_CYAN);
}