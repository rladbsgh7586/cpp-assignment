# 공통 라이브러리 + PCH
add_library(common STATIC
    baseclient.cpp
    latencylog.cpp
    logger.cpp
    matchengine.cpp
    util.cpp)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})
target_precompile_headers(common PRIVATE ${CMAKE_SOURCE_DIR}/pch.hpp)
target_link_libraries(common PUBLIC Boost::system Threads::Threads)
