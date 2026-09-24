# ---------------------------------------------------------
# Echo Server
# ---------------------------------------------------------
add_executable(delay_server server_main.cpp)
add_executable(avbw_server bandwidth_server_main.cpp)

# ---------------------------------------------------------
# Client
# ---------------------------------------------------------
add_executable(delay_client client_main.cpp)
add_executable(avbw_client bandwidth_client_main.cpp)

target_link_libraries(delay_client PRIVATE Boost::headers Threads::Threads)
