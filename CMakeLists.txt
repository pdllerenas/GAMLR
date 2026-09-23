cmake_minimum_required(VERSION 3.20)
project(NetworkDelayEstimator 
        VERSION 0.1.0
        DESCRIPTION "GAMLR"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_WARNINGS "-Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow")

set(COMMON_CXX_FLAGS "${COMMON_WARNINGS} -march=native")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG ${COMMON_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined ${COMMON_CXX_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)