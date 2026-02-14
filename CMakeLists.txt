cmake_minimum_required(VERSION 3.14)
project(order-book)

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

find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest) 

add_executable(engine-test test/engine_test.cpp) 
target_compile_definitions(engine-test PRIVATE UNIT_TEST) 
target_link_libraries(engine-test GTest::gtest_main Threads::Threads)
gtest_discover_tests(engine-test)

add_executable(order-test test/order-book-test.cpp) 
target_compile_definitions(order-test PRIVATE UNIT_TEST) 
target_link_libraries(order-test GTest::gtest_main Threads::Threads)
gtest_discover_tests(order-test)

add_executable(order-book src/main.cpp)
target_link_libraries(order-book PRIVATE Threads::Threads)
