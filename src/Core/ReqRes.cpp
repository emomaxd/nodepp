#include "ReqRes.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


std::ostream&
operator<<(std::ostream& os, const Request& request)
{
    os << "Method: " << request.method << "\n";
    os << "Protocol: " << request.protocol << "\n";
    os << "Host: " << request.host << "\n";
    os << "Port: " << request.port << "\n";
    os << "Path: " << request.path << "\n";

    os << "Headers:\n";
    for (const auto& header : request.headers)
    {
        os << "  " << header.first << ": " << header.second << "\n";
    }

    os << "Query Parameters:\n";
    for (const auto& param : request.query)
    {
        os << "  " << param.first << ": " << param.second << "\n";
    }

    os << "Body:\n" << request.body << "\n";
    return os;
}

#pragma region Request

Request::Request(const std::string& httpRequest)
{
    parseRequest(httpRequest);
}

std::string
Request::getHeader(const std::string& key) const
{
    auto it = headers.find(key);
    return it != headers.end() ? it->second : "";
}

std::string
Request::queryParam(const std::string& key) const
{
    auto it = query.find(key);
    return it != query.end() ? it->second : "";
}

void
Request::parseRequest(const std::string& httpRequest)
{
    std::istringstream requestStream(httpRequest);
    std::string requestLine;

    std::getline(requestStream, requestLine);
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method >> url;

    extractUrlComponents(url);

    size_t queryPos = url.find('?');
    if (queryPos != std::string::npos)
    {
        path = url.substr(0, queryPos);
        std::string queryString = url.substr(queryPos + 1);
        parseQueryParams(queryString);
    } 
    else
    {
        path = url;
    }

    std::string headerLine;
    while (std::getline(requestStream, headerLine) && !headerLine.empty()) 
    {
        size_t separator = headerLine.find(':');
        if (separator != std::string::npos)
        {
            std::string key = trim(headerLine.substr(0, separator));
            std::string value = trim(headerLine.substr(separator + 1));
            headers[key] = value;
        }
    }

    std::string remaining;
    while (std::getline(requestStream, remaining)) 
    {
        body += remaining + "\n";
    }
    if (!body.empty()) 
    {
        body.pop_back();
    }
}

void
Request::extractUrlComponents(const std::string& url) 
{
    if (url.find("https://") == 0)
    {
        protocol = "HTTPS";
        port = 443;
    } 
    else
    {
        protocol = "HTTP";
        port = 80;
    }

    std::size_t protocolEnd = url.find("://");
    std::size_t hostStart = (protocolEnd != std::string::npos) ? protocolEnd + 3 : 0;
    std::size_t portStart = url.find(':', hostStart);
    std::size_t pathStart = url.find('/', hostStart);

    if (portStart != std::string::npos && (pathStart == std::string::npos || portStart < pathStart)) 
    {
        host = url.substr(hostStart, portStart - hostStart);
        port = std::stoi(url.substr(portStart + 1, pathStart - portStart - 1));
    } 
    else 
    {
        if (pathStart != std::string::npos) 
        {
            host = url.substr(hostStart, pathStart - hostStart);
        } 
        else 
        {
            host = url.substr(hostStart);
        }
    }
}

void
Request::parseQueryParams(const std::string& queryString) 
{
    std::istringstream queryStream(queryString);
    std::string pair;
    while (std::getline(queryStream, pair, '&')) 
    {
        size_t separator = pair.find('=');
        if (separator != std::string::npos) 
        {
            std::string key = trim(pair.substr(0, separator));
            std::string value = trim(pair.substr(separator + 1));
            query[key] = value;
        }
    }
}

std::string
Request::trim(const std::string& str) 
{
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}

#pragma endregion

#pragma region Response

Response::Response() : statusCode(200) 
{
}

Response& 
Response::status(int code) 
{
    statusCode = code;
    return *this;
}

Response& 
Response::setHeader(const std::string& key, const std::string& value) 
{
    headers[key] = value;
    return *this;
}

Response& 
Response::send(const std::string& responseBody) 
{
    body = responseBody;
    setHeader("Content-Length", std::to_string(body.size()));
    setHeader("Content-Type", "text/plain");
    return *this;
}

Response& 
Response::json(const std::string& jsonBody) 
{
    body = jsonBody;
    setHeader("Content-Length", std::to_string(body.size()));
    setHeader("Content-Type", "application/json");
    return *this;
}

Response& 
Response::sendFile(const std::string& filePath) 
{
    body = readFile(filePath);
    setHeader("Content-Type", "text/html");
    setHeader("Content-Length", std::to_string(body.size()));
    return *this;
}

std::string 
Response::toHttpResponse() const 
{
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << getStatusMessage() << "\r\n";
    for (const auto& header : headers) 
    {
        response << header.first << ": " << header.second << "\r\n";
    }
    response << "\r\n" << body;
    return response.str();
}

std::string 
Response::readFile(const std::string& filePath) 
{
    std::ifstream file(filePath);
    if (!file.is_open()) 
    {
        std::cerr << "Could not open file: " << filePath << "\n";
        return "File not found";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string 
Response::getStatusMessage() const 
{
    switch (statusCode) 
    {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

#pragma endregion
