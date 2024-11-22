#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "ThreadPool.h"
#include "ReqRes.h"
#include "flog.h"

class App 
{
public:
    using RouteHandler = std::function<void(const Request&, Response&)>;

    App();
    explicit App(size_t ThreadCount);

    void listen(int port, std::function<void()> onStart);
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);

private:
    ThreadPool threadPool;
    std::unordered_map<std::string, RouteHandler> routes;
    std::mutex routesMutex;
    int serverSocket;

    void handleRequest(int clientSocket);

    bool createServerSocket();
    bool configureServerSocket();
    bool bindServerSocket(int port);
};