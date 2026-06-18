cmake_minimum_required(VERSION 3.20)

# 소스트리 오염방지: in-source 빌드 금지
# cmake 부산물 직접 삭제필요
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "cmake -B build && cmake --build build  를 사용하세요.\n")
endif()

project(cpp_assignment CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_compile_options(-Wall -Werror -Wextra)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # compile_commands.json 생성

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(consumer)
