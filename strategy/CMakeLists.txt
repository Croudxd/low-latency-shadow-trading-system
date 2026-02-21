cmake_minimum_required(VERSION 3.14)
project(strategy)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

include_directories(include)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest)
# add_executable(strategy src/main.cpp) 
# target_compile_definitions(engine-test PRIVATE UNIT_TEST) 
# target_link_libraries(engine-test GTest::gtest_main Threads::Threads)
# gtest_discover_tests(engine-test)
#
add_executable(engine_test test/test_engine.cpp) 
target_compile_definitions(engine_test PRIVATE UNIT_TEST) 
target_link_libraries(engine_test GTest::gtest_main Threads::Threads)
gtest_discover_tests(engine_test)

# add_executable(micro-bench benchmark/micro_bench.cpp)
# target_link_libraries(micro-bench benchmark::benchmark Threads::Threads)

add_executable(strategy src/main.cpp)
add_executable(viewer src/viewer.cpp)
target_link_libraries(strategy PRIVATE Threads::Threads)
