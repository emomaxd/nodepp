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