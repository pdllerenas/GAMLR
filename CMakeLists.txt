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

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
enable_testing()
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)
if(TARGET benchmark)
    target_compile_options(benchmark PRIVATE "-Wno-error" "-Wno-conversion")
endif()
if(TARGET benchmark_main)
    target_compile_options(benchmark_main PRIVATE "-Wno-error" "-Wno-conversion")
endif()

add_subdirectory(src)
add_subdirectory(tests)