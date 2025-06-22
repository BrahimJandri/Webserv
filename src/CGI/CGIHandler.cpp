#include "CGIHandler.hpp"
#include "../Utils/Logger.hpp" // Assuming you have a Logger for debug/error output
                               // Make sure your Logger defines macros like LOG_ERROR if used

#include <iostream>   // For std::cerr, std::cout
#include <unistd.h>   // For fork, execve, pipe, close, read, write, dup2
#include <sys/wait.h> // For waitpid
#include <vector>     // For std::vector
#include <cstring>    // For strdup, strlen, strerror
#include <sstream>    // For std::istringstream, std::ostringstream (for to_string_c98)
#include <cstdio>     // For perror
#include <cstdlib>    // For exit, EXIT_FAILURE, getenv
#include <cctype>     // For std::toupper (C++98)
#include <errno.h>    // For errno

// to_string_c98 function (if not already defined in a common utility header and included)
// This is a C++98 compliant way to convert size_t to std::string.
std::string to_string_c98(size_t val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

// Constructor
CGIHandler::CGIHandler(const std::string &cgi_bin_path) : _cgi_bin_path(cgi_bin_path)
{
    // Ensure cgi_bin_path ends with a slash for easier path concatenation
    if (!_cgi_bin_path.empty() && _cgi_bin_path[_cgi_bin_path.length() - 1] != '/')
    {
        _cgi_bin_path += "/";
    }
}

// Destructor
CGIHandler::~CGIHandler()
{
    // No dynamic memory in _env map itself, so no special cleanup needed here.
    // Cleanup of char** arrays is handled by freeCstrArray when they are used.
}

// Helper to convert std::map<std::string, std::string> to char** for execve
// Allocates memory for each string and the array itself, needs to be freed.
char **CGIHandler::mapToCstrArray(const std::map<std::string, std::string> &m)
{
    char **envp = new char *[m.size() + 1]; // +1 for the NULL terminator
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); ++it, ++i)
    {
        std::string env_str = it->first + "=" + it->second;
        envp[i] = strdup(env_str.c_str()); // strdup allocates memory
        if (envp[i] == NULL)
        {
            // Handle allocation failure: free previously allocated memory and return NULL
            for (int j = 0; j < i; ++j)
            {
                free(envp[j]);
            }
            delete[] envp;
            return NULL;
        }
    }
    envp[m.size()] = NULL; // Null-terminate the array
    return envp;
}

// Helper to free the memory allocated by mapToCstrArray
void CGIHandler::freeCstrArray(char **arr)
{
    if (arr)
    {
        for (int i = 0; arr[i] != NULL; ++i)
        {
            free(arr[i]); // Free individual strings
        }
        delete[] arr; // Free the array itself
    }
}

// Helper to read all content from a file descriptor
std::string CGIHandler::readFd(int fd)
{
    std::string content;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        content.append(buffer, static_cast<size_t>(bytes_read)); // Use static_cast for C++98 compatibility
    }
    if (bytes_read == -1)
    {
        // LOG_ERROR("CGI readFd error: " << strerror(errno));
        std::cerr << "CGI readFd error: " << strerror(errno) << std::endl;
    }
    return content;
}

// Helper for error response creation (using Response class)
// Assumes Response class has appropriate setters and addHeader methods.
Response CGIHandler::createErrorResponse(int status_code, const std::string &message)
{
    Response response;
    response.setStatusCode(status_code);
    std::string status_text;
    if (status_code == 404)
        status_text = "Not Found";
    else if (status_code == 500)
        status_text = "Internal Server Error";
    else if (status_code == 502)
        status_text = "Bad Gateway"; // Specific for CGI issues
    else
        status_text = "Error"; // Default fallback
    response.setStatusText(status_text);

    // Provide a simple HTML body for the error
    std::string error_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) + "</title></head><body><h1>Error " + to_string_c98(status_code) + " - " + status_text + "</h1><p>" + message + "</p></body></html>";
    response.setBody(error_body);
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Content-Length", to_string_c98(error_body.length()));
    response.addHeader("Connection", "close"); // Always close after error
    return response;
}

// Set CGI environment variables
void CGIHandler::setEnv(const requestParser &request, const std::string &script_path, const std::string &server_name, int server_port)
{
    _env.clear(); // Clear previous environment variables

    // Standard CGI variables
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_PROTOCOL"] = request.getHttpVersion(); // e.g., HTTP/1.1
    _env["SERVER_SOFTWARE"] = "webserv/1.0";            // Your server's name and version
    _env["REQUEST_METHOD"] = request.getMethod();

    // SCRIPT_NAME and PATH_INFO determination
    // script_path here is the *requested URI* path, e.g., "/cgi-bin/myscript.pl/extra/info"
    // We need to determine which part is the actual script, and which is PATH_INFO
    // This logic might need refinement based on your config's specific CGI mappings.
    size_t script_name_end_pos = script_path.length(); // Default: entire path is script name
    // A more robust way might be to check if the path (or part of it) exists as an executable file
    // For now, let's assume the actual script file ends before the first '/' after the cgi-bin part
    // For example, if script_path is "/cgi-bin/script.pl/extra/path", we assume "script.pl" is the script.

    size_t cgi_bin_len = std::string("/cgi-bin/").length(); // Assuming common /cgi-bin/ convention
    size_t first_slash_after_cgibin = std::string::npos;
    if (script_path.length() > cgi_bin_len)
    {
        first_slash_after_cgibin = script_path.find('/', cgi_bin_len);
    }

    if (first_slash_after_cgibin != std::string::npos)
    {
        _env["SCRIPT_NAME"] = script_path.substr(0, first_slash_after_cgibin);
        _env["PATH_INFO"] = script_path.substr(first_slash_after_cgibin);
    }
    else
    {
        _env["SCRIPT_NAME"] = script_path;
        _env["PATH_INFO"] = "";
    }

    _env["REQUEST_URI"] = request.getPath(); // The full path requested by client

    // Query string for GET requests
    size_t query_pos = request.getPath().find('?');
    if (query_pos != std::string::npos)
    {
        _env["QUERY_STRING"] = request.getPath().substr(query_pos + 1);
        // Ensure SCRIPT_NAME doesn't include the query string
        if (!_env["SCRIPT_NAME"].empty() && _env["SCRIPT_NAME"].find('?') != std::string::npos)
        {
            _env["SCRIPT_NAME"] = _env["SCRIPT_NAME"].substr(0, _env["SCRIPT_NAME"].find('?'));
        }
    }
    else
    {
        _env["QUERY_STRING"] = "";
    }

    _env["SERVER_NAME"] = server_name;                                     // e.g., localhost
    _env["SERVER_PORT"] = to_string_c98(static_cast<size_t>(server_port)); // Cast to size_t for to_string_c98

    // Remote client information (placeholders for now)
    _env["REMOTE_ADDR"] = "127.0.0.1"; // Assuming local client
    _env["REMOTE_HOST"] = "localhost"; // Assuming local client

    // HTTP Headers from client request (prefixed with HTTP_)
    const std::map<std::string, std::string> &headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string env_key = "HTTP_" + it->first;
        // Convert header name to uppercase and replace hyphens with underscores
        for (size_t i = 0; i < env_key.length(); ++i)
        {
            if (env_key[i] == '-')
            {
                env_key[i] = '_';
            }
            else
            {
                // Use static_cast to prevent warnings when using toupper with char
                env_key[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(env_key[i])));
            }
        }
        _env[env_key] = it->second;
    }

    // Content-related variables for POST requests
    if (request.getMethod() == "POST")
    {
        if (request.getHeaders().count("Content-Type"))
        {
            _env["CONTENT_TYPE"] = request.getHeaders().at("Content-Type");
        }
        else
        {
            _env["CONTENT_TYPE"] = "";
        }
        _env["CONTENT_LENGTH"] = to_string_c98(request.getBody().length());
    }
    else
    {
        _env["CONTENT_TYPE"] = "";
        _env["CONTENT_LENGTH"] = "";
    }

    // AUTH_TYPE, REMOTE_USER, REMOTE_IDENT, etc. could be added here for authentication
}

// Main execution function
Response CGIHandler::execute(const requestParser &request, const std::string &script_path, const std::string &server_name, int server_port)
{
    // Set up environment variables for the CGI script
    setEnv(request, script_path, server_name, server_port);

    // Pipes for communication between parent (webserv) and child (CGI script)
    // cgi_in_pipe: Parent writes to this pipe, child reads (CGI's stdin)
    // cgi_out_pipe: Child writes to this pipe, parent reads (CGI's stdout)
    int cgi_in_pipe[2];
    int cgi_out_pipe[2];

    if (pipe(cgi_in_pipe) == -1)
    {
        perror("pipe (cgi_in_pipe) failed");
        // LOG_ERROR("CGI: pipe creation for stdin failed: " << strerror(errno));
        return createErrorResponse(500, "CGI internal error: stdin pipe creation failed.");
    }
    if (pipe(cgi_out_pipe) == -1)
    {
        perror("pipe (cgi_out_pipe) failed");
        // LOG_ERROR("CGI: pipe creation for stdout failed: " << strerror(errno));
        close(cgi_in_pipe[0]);
        close(cgi_in_pipe[1]); // Clean up first pipe
        return createErrorResponse(500, "CGI internal error: stdout pipe creation failed.");
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        // LOG_ERROR("CGI: fork failed: " << strerror(errno));
        close(cgi_in_pipe[0]);
        close(cgi_in_pipe[1]);
        close(cgi_out_pipe[0]);
        close(cgi_out_pipe[1]);
        return createErrorResponse(500, "CGI internal error: fork failed.");
    }
    else if (pid == 0) // Child process (CGI script)
    {
        // Close the ends of pipes not used by the child
        close(cgi_in_pipe[1]);  // Child will read from cgi_in_pipe[0]
        close(cgi_out_pipe[0]); // Child will write to cgi_out_pipe[1]

        // Redirect child's stdin to read from the pipe
        if (dup2(cgi_in_pipe[0], STDIN_FILENO) == -1)
        {
            perror("dup2 STDIN failed");
            exit(EXIT_FAILURE);
        }
        close(cgi_in_pipe[0]); // Close original pipe fd after dup2

        // Redirect child's stdout to write to the pipe
        if (dup2(cgi_out_pipe[1], STDOUT_FILENO) == -1)
        {
            perror("dup2 STDOUT failed");
            exit(EXIT_FAILURE);
        }
        close(cgi_out_pipe[1]); // Close original pipe fd after dup2

        // Construct arguments for execve
        // The first argument to execve (argv[0]) is conventionally the script name itself.
        // The actual file path of the CGI script on the file system.
        // This assumes that the script_path (e.g., "/cgi-bin/script.py")
        // maps to _cgi_bin_path + "script.py" on the file system.
        std::string full_script_filesystem_path = _cgi_bin_path;
        size_t last_slash_pos = script_path.rfind('/');
        if (last_slash_pos != std::string::npos)
        {
            full_script_filesystem_path += script_path.substr(last_slash_pos + 1);
        }
        else
        {
            full_script_filesystem_path += script_path; // If no slash, script_path is just the name
        }

        // We also need to check if the script is an interpreter, e.g., for Python scripts.
        // For simplicity, we assume the script itself is executable or has a shebang.
        // You might need to prepend "/usr/bin/python" etc. for interpreters.
        // For example: if script_path ends with .py, full_script_exec_path = "/usr/bin/python", argv_vec.push_back(full_script_filesystem_path.c_str());
        // For now, we execute the script directly.

        // argv for execve. argv[0] is the script name.
        std::vector<char *> argv_vec;
        argv_vec.push_back(strdup(full_script_filesystem_path.c_str()));
        // If your CGI script takes arguments from PATH_INFO or QUERY_STRING as argv, add them here.
        // For typical CGI, these are passed via environment variables.
        argv_vec.push_back(NULL); // Null-terminate argv array

        char **envp = mapToCstrArray(_env);
        if (envp == NULL)
        {
            // LOG_ERROR("CGI: Failed to allocate environment array.");
            std::cerr << "CGI: Failed to allocate environment array." << std::endl;
            for (size_t i = 0; i < argv_vec.size(); ++i)
            { // Free argv memory
                free(argv_vec[i]);
            }
            exit(EXIT_FAILURE);
        }

        // Execute the CGI script
        // execve takes: path_to_executable, argv_array, envp_array
        if (execve(argv_vec[0], argv_vec.data(), envp) == -1)
        {
            perror(("execve failed for " + full_script_filesystem_path).c_str());
            // LOG_ERROR("CGI: execve failed for " << full_script_filesystem_path << ": " << strerror(errno));
            // Child process must exit after execve failure
            freeCstrArray(envp); // Free allocated envp memory
            for (size_t i = 0; i < argv_vec.size(); ++i)
            { // Free argv memory
                free(argv_vec[i]);
            }
            exit(EXIT_FAILURE); // Important: exit child process on failure
        }
    }
    else // Parent process (Webserv)
    {
        // Close the ends of pipes not used by the parent
        close(cgi_in_pipe[0]);  // Parent will write to cgi_in_pipe[1]
        close(cgi_out_pipe[1]); // Parent will read from cgi_out_pipe[0]

        // If it's a POST request, write the body to CGI's stdin
        if (request.getMethod() == "POST" && !request.getBody().empty())
        {
            // Write the request body to the CGI script's stdin
            // Use loop for robustness in case write doesn't write all bytes at once
            const std::string &body = request.getBody();
            size_t total_written = 0;
            ssize_t bytes_written;
            while (total_written < body.length())
            {
                bytes_written = write(cgi_in_pipe[1], body.c_str() + total_written, body.length() - total_written);
                if (bytes_written == -1)
                {
                    perror("write to CGI stdin failed");
                    // LOG_ERROR("CGI: write to stdin pipe failed: " << strerror(errno));
                    close(cgi_in_pipe[1]);
                    close(cgi_out_pipe[0]);
                    return createErrorResponse(500, "CGI internal error: failed to write request body.");
                }
                total_written += bytes_written;
            }
        }
        close(cgi_in_pipe[1]); // Done writing to CGI stdin, close write end to signal EOF to child

        // Read CGI output from stdout pipe
        std::string cgi_output = readFd(cgi_out_pipe[0]);
        close(cgi_out_pipe[0]); // Done reading from CGI stdout

        // Wait for CGI child process to finish
        int status;
        // WNOHANG could be used for non-blocking wait, but for simple CGI, blocking is fine.
        pid_t result = waitpid(pid, &status, 0);

        if (result == -1)
        {
            perror("waitpid failed");
            // LOG_ERROR("CGI: waitpid failed: " << strerror(errno));
            return createErrorResponse(500, "CGI internal error: waitpid failed.");
        }

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) != 0)
            {
                // LOG_ERROR("CGI script exited with non-zero status: " << WEXITSTATUS(status));
                return createErrorResponse(502, "CGI script exited with an error (status " + to_string_c98(static_cast<size_t>(WEXITSTATUS(status))) + ").");
            }
        }
        else if (WIFSIGNALED(status))
        {
            // LOG_ERROR("CGI script terminated by signal: " << WTERMSIG(status));
            return createErrorResponse(502, "CGI script terminated by signal " + to_string_c98(static_cast<size_t>(WTERMSIG(status))) + ".");
        }
        else
        {
            // Child process did not exit or terminate by signal (e.g., still running, stopped)
            // This scenario should ideally not happen if waitpid successfully waited for exit/signal
            return createErrorResponse(502, "CGI script did not terminate cleanly.");
        }

        // Parse CGI output to construct HTTP response
        // CGI output typically starts with headers like "Content-type: text/html\r\n\r\n" followed by body
        size_t header_end = cgi_output.find("\r\n\r\n");
        if (header_end == std::string::npos)
        {
            // LOG_ERROR("CGI output missing header-body separator '\\r\\n\\r\\n'");
            return createErrorResponse(502, "CGI script produced malformed output (missing '\\r\\n\\r\\n').");
        }

        std::string cgi_headers_str = cgi_output.substr(0, header_end);
        std::string cgi_body = cgi_output.substr(header_end + 4);

        Response response;
        response.setStatusCode(200); // Default to 200 OK unless CGI headers specify otherwise
        response.setStatusText("OK");

        // Parse CGI headers
        std::istringstream iss_cgi_headers(cgi_headers_str);
        std::string line;
        std::string cgi_status_header_value; // To capture CGI "Status" header

        while (std::getline(iss_cgi_headers, line) && !line.empty())
        {
            // C++98 way to remove trailing \r
            if (!line.empty() && line[line.length() - 1] == '\r')
            {
                line = line.substr(0, line.length() - 1);
            }
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                // Trim leading/trailing whitespace (C++98 compatible)
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "Status")
                { // CGI can set HTTP status code and reason phrase
                    cgi_status_header_value = value;
                }
                else
                {
                    response.addHeader(key, value);
                }
            }
        }

        // Handle CGI-provided "Status" header (e.g., Status: 404 Not Found)
        if (!cgi_status_header_value.empty())
        {
            std::istringstream status_iss(cgi_status_header_value);
            int cgi_status_code;
            status_iss >> cgi_status_code; // Extract numeric code
            std::string cgi_status_text;
            std::getline(status_iss, cgi_status_text); // Get the rest as status text
            if (!cgi_status_text.empty() && cgi_status_text[0] == ' ')
            {
                cgi_status_text = cgi_status_text.substr(1); // Remove leading space
            }
            response.setStatusCode(cgi_status_code);
            response.setStatusText(cgi_status_text);
        }

        response.setBody(cgi_body);
        response.addHeader("Content-Length", to_string_c98(cgi_body.length()));

        // If CGI didn't provide Content-Type, you might want to default it
        if (response.getHeaders().find("Content-Type") == response.getHeaders().end())
        {
            response.addHeader("Content-Type", "text/html"); // Default to HTML
        }
        response.addHeader("Connection", "close"); // Standard for end of request

        return response;
    }
}
