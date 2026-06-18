add_executable(consumer
    main_consumer.cpp
    consumer.cpp)
target_link_libraries(consumer PRIVATE common)
target_precompile_headers(consumer REUSE_FROM common)  # common 의 PCH 재사용
