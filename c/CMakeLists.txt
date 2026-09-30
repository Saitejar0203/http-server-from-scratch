cmake_minimum_required(VERSION 3.13)

project(codecrafters-http-server)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(http-server ${SOURCE_FILES})

find_package(Threads REQUIRED)
target_link_libraries(http-server PRIVATE Threads::Threads)
target_compile_options(http-server PRIVATE -Wall -Wextra -Wpedantic)

find_package(ZLIB REQUIRED)
target_link_libraries(http-server PRIVATE ZLIB::ZLIB)
