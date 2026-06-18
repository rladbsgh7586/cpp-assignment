# 서버 실행파일
add_executable(server
    server_main.cpp
    server.cpp)
target_link_libraries(server PRIVATE common)
target_precompile_headers(server REUSE_FROM common)  # common 의 PCH 재사용
