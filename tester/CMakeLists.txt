cmake_minimum_required(VERSION 3.8)

project(tester)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC
    ${CMAKE_SOURCE_DIR}/tester/*.cpp
    ${CMAKE_SOURCE_DIR}/tester/*.h
)

add_executable(tester ${SRC})

add_compile_options(-Wall -Wextra -Wpedantic -O2 -march=native -flto)

target_include_directories(tester PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(tester
    m
    nodepp
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
