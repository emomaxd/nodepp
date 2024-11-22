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
        std::cout << "Server started." << std::endl;
    });

    return 0;
}
