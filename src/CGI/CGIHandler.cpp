#include "CGIHandler.hpp"

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() {}

char **CGIHandler::buildEnvArray(const std::map<std::string, std::string> &envVars)
{
    char **env = new char *[envVars.size() + 1];
    size_t i = 0;
    for (const auto &pair : envVars)
    {
        std::string entry = pair.first + "=" + pair.second;
        env[i] = new char[entry.size() + 1];
        std::strcpy(env[i], entry.c_str());
        i++;
    }
    env[i] = nullptr;
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

std::string CGIHandler::execute(const std::string &scriptPath,
                                const std::string &requestMethod,
                                const std::string &requestBody,
                                const std::map<std::string, std::string> &envVars)
{
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1)
    {
        throw std::runtime_error("pipe() failed");
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0)
    {
        // Child process

        // Redirect stdin
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);

        // Redirect stdout
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[1]);
        close(stdout_pipe[0]);

        // Change to script directory
        std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        chdir(scriptDir.c_str());

        // Build env vars
        char **envp = buildEnvArray(envVars);

        const char *interpreter = "/usr/bin/bash"; // or whatever your config sets
        const char *argv[] = {interpreter, scriptPath.c_str(), nullptr};
        execve(interpreter, (char *const *)argv, envp);

        // If exec fails
        perror("execve");
        freeEnvArray(envp);
        exit(1);
    }
    else
    {
        // Parent process

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // Write to child's stdin (for POST body)
        if (requestMethod == "POST" && !requestBody.empty())
        {
            write(stdin_pipe[1], requestBody.c_str(), requestBody.size());
        }
        close(stdin_pipe[1]); // EOF for child

        // Read child's stdout
        std::ostringstream output;
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
        {
            output.write(buffer, bytesRead);
        }
        close(stdout_pipe[0]);

        // Wait for child to finish
        int status;
        waitpid(pid, &status, 0);

        return output.str();
    }
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

    auto headers = req.getHeaders();
    if (req.getMethod() == "POST")
    {
        if (headers.find("Content-Length") != headers.end())
            env["CONTENT_LENGTH"] = headers.at("Content-Length");
        else
            env["CONTENT_LENGTH"] = std::to_string(req.getBody().size());

        if (headers.find("Content-Type") != headers.end())
            env["CONTENT_TYPE"] = headers.at("Content-Type");
        else
            env["CONTENT_TYPE"] = "";
    }

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second;

        // Transform header name to CGI env format, e.g. "User-Agent" => "HTTP_USER_AGENT"
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c)
                       {
                           if (c == '-')
                               return '_';
                           else
                               return static_cast<char>(std::toupper(c)); // Cast to char fixes it
                       });

        env["HTTP_" + key] = value;
    }

    return env;
}
