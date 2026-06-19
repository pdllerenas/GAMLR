
add_executable(NetworkDelayEstimator main.cpp)

target_link_libraries(NetworkDelayEstimator PRIVATE Boost::headers Threads::Threads)

target_include_directories(market_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
