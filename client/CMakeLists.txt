add_executable(client
    main_client.cpp
    client.cpp)
target_link_libraries(client PRIVATE common)
