cmake_minimum_required(VERSION 3.14)
project(tg-autoresponder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tg-autoresponder
    src/main.cpp
    src/config.cpp
    src/handler.cpp
    src/utils.cpp
)

target_include_directories(tg-autoresponder PRIVATE src)

# No linking needed — tdjson.dll is loaded dynamically at runtime via LoadLibrary
