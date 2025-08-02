#include "CGI.hpp"

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() {}

std::string to_string_c98(size_t val);

char **CGIHandler::buildEnvArray(const std::map<std::string, std::string> &envVars)
{
    char **env = new char *[envVars.size() + 1];
    size_t i = 0;
    std::map<std::string, std::string>::const_iterator it;
    for (it = envVars.begin(); it != envVars.end(); ++it)
    {
        std::string entry = it->first + "=" + it->second;
        env[i] = new char[entry.size() + 1];
        std::strcpy(env[i], entry.c_str());
        ++i;
    }
    env[i] = 0;
    return env;
}

void CGIHandler::freeEnvArray(char **env)
{
    if (!env)
        return;
    for (int i = 0; env[i]; ++i)
        delete[] env[i];
    delete[] env;
}

std::string CGIHandler::execute(const std::string &scriptPath, const requestParser &request, const std::map<std::string, std::string> &envVars, const std::string &interpreter)
{
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1)
        throw std::runtime_error("pipe() failed");

    pid_t pid = fork();
    if (pid < 0)
    {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0)
    {
        // CHILD
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        if (chdir(scriptDir.c_str()) == -1)
        {
            perror("chdir");
            exit(EXIT_FAILURE);
        }

        char **envp = buildEnvArray(envVars);
        std::string executableName = interpreter;
        std::string scriptFileName = scriptPath.substr(scriptPath.find_last_of('/') + 1);

        std::vector<const char *> argv_vec;
        argv_vec.push_back(executableName.c_str());
        if (interpreter != scriptPath)
            argv_vec.push_back(scriptFileName.c_str());
        argv_vec.push_back(NULL);

        execve(executableName.c_str(), const_cast<char *const *>(argv_vec.data()), envp);
        perror("execve");
        freeEnvArray(envp);
        exit(EXIT_FAILURE);
    }
    else
    {
        // PARENT
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        if (request.getMethod() == "POST" && !request.getBody().empty())
        {
            write(stdin_pipe[1], request.getBody().c_str(), request.getBody().size());
        }
        close(stdin_pipe[1]);

        std::ostringstream output;
        char buffer[4096];
        ssize_t bytesRead;

        int status = 0;
        time_t start_time = time(NULL);
        const int timeout = 5; 

        while (42)
        {
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
                if (time(NULL) - start_time >= timeout)
                {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    close(stdout_pipe[0]);
                    throw std::runtime_error("CGI script timed out");
                }
                usleep(100000); 
            }
            else if (result > 0)
            {
                while ((bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
                {
                    output.write(buffer, bytesRead);
                }
                break;
            }
            else
            {
                close(stdout_pipe[0]);
                throw std::runtime_error("waitpid() failed");
            }
        }

        close(stdout_pipe[0]);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            throw std::runtime_error("CGI script exited with error");
        }

        return output.str();
    }
}

char to_cgi_char(char c)
{
    if (c == '-')
        return '_';
    return std::toupper(static_cast<unsigned char>(c));
}

std::map<std::string, std::string> CGIHandler::prepareCGIEnv(const requestParser &req)
{
    std::map<std::string, std::string> env;

    env["REQUEST_METHOD"] = req.getMethod();
    env["SCRIPT_NAME"] = req.getPath();

    env["SERVER_PROTOCOL"] = req.getHttpVersion();

    std::string path = req.getPath();
    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
    {
        env["QUERY_STRING"] = path.substr(qpos + 1);
        env["SCRIPT_NAME"] = path.substr(0, qpos);
    }
    else
    {
        env["QUERY_STRING"] = "";
    }

    std::map<std::string, std::string> headers = req.getHeaders();
    if (req.getMethod() == "POST")
    {
        if (headers.find("Content-Length") != headers.end())
            env["CONTENT_LENGTH"] = headers.at("Content-Length");
        else
            env["CONTENT_LENGTH"] = to_string_c98(req.getBody().size());

        if (headers.find("Content-Type") != headers.end())
            env["CONTENT_TYPE"] = headers.at("Content-Type");
        else
            env["CONTENT_TYPE"] = "";
    }

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";

    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second;

        std::transform(key.begin(), key.end(), key.begin(), to_cgi_char);

        env["HTTP_" + key] = value;
    }
    return env;
}

std::string CGIHandler::handleCGI(const std::string &scriptPath, const requestParser &request, std::string interpreter, int client_fd)
{
    Response response;
    std::map<std::string, std::string> env = CGIHandler::prepareCGIEnv(request);

    try
    {
        std::string cgiOutput = execute(scriptPath, request, env, interpreter);

        size_t pos = cgiOutput.find("\r\n\r\n");
        size_t separator_len = 4;
        if (pos == std::string::npos)
        {
            pos = cgiOutput.find("\n\n");
            separator_len = 2;
        }

        if (pos != std::string::npos)
        {
            std::string headerPart = cgiOutput.substr(0, pos);
            std::string bodyPart = cgiOutput.substr(pos + separator_len);

            std::istringstream headerStream(headerPart);
            std::string line;
            response.setStatus(200, "OK");

            while (std::getline(headerStream, line))
            {
                if (line.empty())
                    break;

                size_t colon = line.find(':');
                if (colon != std::string::npos)
                {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    key.erase(key.find_last_not_of(" \t\r\n") + 1);
                    value.erase(0, value.find_first_not_of(" \t\r\n"));
                    value.erase(value.find_last_not_of(" \t\r\n") + 1);

                    if (key == "Set-Cookie")
                    {
                        response.addHeader("Set-Cookie", value);
                    }
                    else
                    {
                        response.addHeader(key, value);
                    }
                }
            }

            response.setBody(bodyPart);

            if (response.getHeaders().find("Content-Type") == response.getHeaders().end())
                response.addHeader("Content-Type", "text/html");

            response.addHeader("Content-Length", to_string_c98(bodyPart.size()));
        }
        else
        {
            response.setStatus(200, "OK");
            response.setBody(cgiOutput);
            response.addHeader("Content-Type", "text/html");
            response.addHeader("Content-Length", to_string_c98(cgiOutput.size()));
        }
        return response.toString();
    }
    catch (const std::exception &e)
    {
        if(e.what() == std::string("CGI script timed out"))
        {
            Utils::log("CGI script timed out for " + scriptPath, AnsiColor::BOLD_RED);
            send_error_response(client_fd, 504, "Gateway Timeout", response.serverConfig);
            response.setStatus(504, "Gateway Timeout");
        }
        else if (e.what() == std::string("CGI script exited with error"))
        {
            send_error_response(client_fd, 500, "Internal Server Error", response.serverConfig);
            response.setStatus(500, "Internal Server Error");
        }
        else
        {
            Utils::log("CGI execution failed for " + scriptPath + ": " + e.what(), AnsiColor::BOLD_RED);
            send_error_response(client_fd, 500, "Internal Server Error", response.serverConfig);
            response.setStatus(500, "Internal Server Error");
        }
        std::string err = "CGI Error: ";
        err += e.what();
        response.setBody(err);
        response.addHeader("Content-Type", "text/html");
        response.addHeader("Content-Length", to_string_c98(err.size()));
    }

    return response.toString();
}
