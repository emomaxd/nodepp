#include "App.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>
#include <functional>
#include <unordered_map>

App::App() 
    : threadPool(2 * std::thread::hardware_concurrency()), serverSocket(-1)
{
    /* Initialize ThreadPool with double the hardware thread count */
}

App::App(size_t ThreadCount)
    : threadPool(ThreadCount), serverSocket(-1)
{
}

void
App::listen(int port, std::function<void()> onStart)
{
    if (!createServerSocket() || !configureServerSocket() || !bindServerSocket(port))
    {
        std::cerr << "Failed to initialize server socket\n";
        return;
    }

    if (::listen(serverSocket, 5) < 0)
    {
        std::cerr << "Listen failed\n";
        close(serverSocket);
        return;
    }

    if (onStart)
    {
        onStart();
    }

    while (true)
    {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0)
        {
            threadPool.enqueue([this, clientSocket] 
            {
                handleRequest(clientSocket);
            });
        }
    }

    close(serverSocket);
}

void
App::get(const std::string& path, RouteHandler handler)
{
    std::lock_guard<std::mutex> lock(routesMutex);
    routes[path] = handler;
}

void
App::post(const std::string& path, RouteHandler handler)
{
    std::lock_guard<std::mutex> lock(routesMutex);
    routes[path] = handler;
}

void
App::handleRequest(int clientSocket)
{
    constexpr uint16_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, BUFFER_SIZE);

    if (bytesRead < 0)
    {
        std::cerr << "Read failed\n";
        close(clientSocket);
        return;
    }

    std::string requestStr(buffer);
    Request req(requestStr);
    
    Response res;
    res.statusCode = 404; // Default to 404 Not Found

    {
        std::lock_guard<std::mutex> lock(routesMutex);
        auto it = routes.find(req.path);
        if (it != routes.end())
        {
            it->second(req, res); // Call the route handler
        }
    }

    if (res.body.empty())
    {
        res.status(404).send("Not Found");
    }

    send(clientSocket, res.toHttpResponse().c_str(), res.toHttpResponse().size(), 0);
    close(clientSocket);
}

bool
App::createServerSocket()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "Could not create socket\n";
        return false;
    }
    return true;
}

bool
App::configureServerSocket()
{
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Could not set socket options\n";
        close(serverSocket);
        return false;
    }
    return true;
}

bool
App::bindServerSocket(int port)
{
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "Bind failed\n";
        close(serverSocket);
        return false;
    }
    return true;
}
