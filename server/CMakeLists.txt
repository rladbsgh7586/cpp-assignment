# 서버 실행파일
add_executable(server
    server_main.cpp
    server.cpp)
target_link_libraries(server PRIVATE common)
