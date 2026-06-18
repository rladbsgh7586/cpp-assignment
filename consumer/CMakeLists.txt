add_executable(consumer
    main_consumer.cpp
    consumer.cpp)
target_link_libraries(consumer PRIVATE common)
