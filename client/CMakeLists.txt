add_executable(client
    main_client.cpp
    client.cpp)
target_link_libraries(client PRIVATE common)
target_precompile_headers(client REUSE_FROM common)  # common 의 PCH 재사용
