cmake_minimum_required(VERSION 3.20)
project(cpp_assignment CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Werror -Wextra)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # compile_commands.json 생성

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
