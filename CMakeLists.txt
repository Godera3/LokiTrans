cmake_minimum_required(VERSION 3.20)
project(LokiTrans LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lokitrans
  src/cli/main.cpp
)

if (MSVC)
  target_compile_options(lokitrans PRIVATE /W4)
else()
  target_compile_options(lokitrans PRIVATE -Wall -Wextra -Wpedantic)
endif()
