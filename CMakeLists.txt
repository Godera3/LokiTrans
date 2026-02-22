cmake_minimum_required(VERSION 3.20)
project(svanipp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(svanipp
    src/main.cpp
    src/net/winsock_init.cpp
    src/net/socket_utils.cpp
    src/transfer/receiver.cpp
)

target_include_directories(svanipp PRIVATE src)

# Winsock on Windows
target_link_libraries(svanipp PRIVATE ws2_32)