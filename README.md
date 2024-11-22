# nodepp - nodejs but c++

nodepp have similar structure and usage to the nodejs, that makes it efficient, easy to use and faster!

## Features
- Serve static files (e.g., `index.html`).
- Define endpoints for HTTP requests (e.g., `/`, `/upload`).
- Integrates `flog` for logging at different levels (trace, debug, info, warn, error, critical).

## Dependencies
- [flog](https://github.com/emomaxd/flog) library for logging.

## Usage
1. Clone the repository.
2. Compile the code with CMake.
    ```bash
    mkdir build && cd build
    cmake ..
    make
    ```
3. Link the library that inside build/src named as `nodepp`(libnodepp.a).
4. Put src/Core inside your include directories.

## Examples
1. Create App instance
    ```cpp
    App app;
    ```
2. Define endpoints
    ```cpp
    app.get("/", [](const Request& req, Response& res) {
        res.sendFile("index.html");
    });

    app.get("/hello", [](const Request& req, Response& res) {
        flog::trace("Entering /hello endpoint");

        res.send("Hello, World!");
    });
    ```
3. Configure which port to listen
    ```cpp
    app.listen(8080, []() {
        flog::info("Server started.");
    });
    ```
4. Easily read and send data from req and res with their member variables and functions
    ```cpp
    flog::debug(req.method);
    res.send("Sending to the client!");
    ```
5. Complete example
    ```cpp
    #include "Core/App.h"

    #include <iostream>

    int main() {
        App app;

        app.get("/", [](const Request& req, Response& res) {
            res.sendFile("index.html");
        });

        app.get("/hello", [](const Request& req, Response& res) {
            // Logging at different levels
            flog::trace("Entering /hello endpoint");          // Trace flog
            flog::debug("Debugging /hello endpoint");          // Debug flog
            flog::info("Handling hello request");              // Info flog
            flog::warn("This is a warning for /hello");        // Warning flog
            flog::error("An error occurred while processing /hello"); // Error flog
            flog::critical("Critical issue in /hello!");      // Critical log

            res.send("Hello, World!");
        });

        app.get("/goodbye", [](const Request& req, Response& res) {
            res.send("Goodbye, World!");
        });

        app.listen(8080, []() {
            flog::info("Server started.");
        });

        return 0;
    }
    ```