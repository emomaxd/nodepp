#pragma once

#include <string>
#include <unordered_map>
#include <vector>


class Request
{
public:
    Request(const std::string& httpRequest);
    std::string getHeader(const std::string& key) const;
    std::string queryParam(const std::string& key) const;
    friend std::ostream& operator<<(std::ostream& os, const Request& request);

public:
    std::string method;
    std::string url;
    std::string protocol;
    std::string host;
    int port;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query;

private:
    void parseRequest(const std::string& httpRequest);
    void extractUrlComponents(const std::string& url);
    void parseQueryParams(const std::string& queryString);
    std::string trim(const std::string& str);
};

class Response
{
public:
    Response();
    Response& status(int code);
    Response& setHeader(const std::string& key, const std::string& value);
    Response& send(const std::string& responseBody);
    Response& json(const std::string& jsonBody);
    Response& sendFile(const std::string& filePath);
    std::string toHttpResponse() const;

public:
    int statusCode;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

private:
    std::string readFile(const std::string& filePath);
    std::string getStatusMessage() const;
};