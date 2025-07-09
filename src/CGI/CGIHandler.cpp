#include "CGIHandler.hpp"

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
    env[i] = 0; // nullptr is not available in C++98
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

std::string CGIHandler::getInterpreterForScript(const std::string &scriptPath)
{
    std::map<std::string, std::string> interpreters;
    interpreters[".py"] = "/usr/bin/python3";
    interpreters[".sh"] = "/bin/bash";
    interpreters[".php"] = "/usr/bin/php";
    interpreters[".js"] = "/usr/bin/node";  // Assuming Node.js for JavaScript
    interpreters[".cgi"] = "/usr/bin/perl"; // Common for CGI scripts

    size_t dot_pos = scriptPath.find_last_of(".");
    if (dot_pos != std::string::npos)
    {
        std::string extension = scriptPath.substr(dot_pos);
        std::map<std::string, std::string>::const_iterator it = interpreters.find(extension);
        if (it != interpreters.end())
        {
            return it->second; // Return the mapped interpreter (e.g., /usr/bin/python3)
        }
    }
    return scriptPath;
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
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0)
    {
        // Child process

        // Redirect stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[1]);
        close(stdout_pipe[0]);

        // Change to script's directory to handle relative paths correctly
        std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        if (chdir(scriptDir.c_str()) == -1)
        {
            perror("chdir");
            exit(EXIT_FAILURE);
        }

        // Prepare environment variables and arguments for execve
        char **envp = buildEnvArray(envVars);

        // --- DYNAMIC INTERPRETER LOGIC START ---

        // 1. Get the correct interpreter for the script
        std::string interpreterPath = getInterpreterForScript(scriptPath);
        std::string executableName = interpreterPath.substr(interpreterPath.find_last_of('/') + 1);

        // 2. Build the argument vector for execve
        std::vector<const char *> argv_vec;
        argv_vec.push_back(executableName.c_str()); // Arg 0: the name of the executable

        // If the interpreter is not the script itself (e.g., python for a .py script),
        // then the script path becomes the first argument to the interpreter.
        if (interpreterPath != scriptPath)
        {
            argv_vec.push_back(scriptPath.c_str());
        }

        argv_vec.push_back(NULL); // The argument list must be null-terminated

        // 3. Execute the script
        execve(interpreterPath.c_str(), (char *const *)argv_vec.data(), envp);

        // --- DYNAMIC INTERPRETER LOGIC END ---

        // If execve fails, this code will be reached
        perror("execve");
        freeEnvArray(envp); // Clean up allocated memory
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // Write request body to child's stdin if it's a POST request
        if (requestMethod == "POST" && !requestBody.empty())
        {
            ssize_t bytesWritten = write(stdin_pipe[1], requestBody.c_str(), requestBody.size());
            if (bytesWritten < 0)
            {
                // Handle write error if necessary
                perror("write to cgi");
            }
        }
        close(stdin_pipe[1]); // Send EOF to child's stdin

        // Read the CGI script's output from its stdout
        std::ostringstream output;
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
        {
            output.write(buffer, bytesRead);
        }
        close(stdout_pipe[0]);

        // Wait for the child process to terminate
        int status;
        waitpid(pid, &status, 0);

        // You might want to check the exit status of the child here
        // to handle CGI errors more gracefully.
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            // CGI script exited with an error
            // Consider logging this or returning a 500 error
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

        // Transform header name to CGI env format
        std::transform(key.begin(), key.end(), key.begin(), to_cgi_char);

        env["HTTP_" + key] = value;
    }

    return env;
}


