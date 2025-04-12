#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

#define PORT 8080

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket failed");
        return 1;
    }

    // 2. Bind to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        return 1;
    }

    // 3. Start listening
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    std::cout << "🟢 Server listening on http://localhost:" << PORT << std::endl;

    // 4. Accept one client connection
    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0)
    {
        perror("accept");
        return 1;
    }

    // 5. Read the HTTP request
    char buffer[4096] = {0};
    read(client_fd, buffer, 4096);
    std::cout << "📥 Received request:\n"
              << buffer << std::endl;

    // 6. Send back a simple HTTP response
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 46\r\n"
        "\r\n"
        "<html><body><h1>Hello, Webserv!</h1></body></html>";

    send(client_fd, response, strlen(response), 0);
    std::cout << "📤 Response sent.\n";

    // 7. Close connections
    close(client_fd);
    close(server_fd);

    return 0;
}
