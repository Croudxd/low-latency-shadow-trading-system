cmake_minimum_required(VERSION 3.10)
project(order-book)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")

include_directories(include)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(engine-test test/engine_test.cpp) 
add_executable(order-test  test/order-book-test.cpp) 

target_compile_definitions(engine-test PRIVATE UNIT_TEST) 
target_compile_definitions(order-test PRIVATE UNIT_TEST) 

target_link_libraries(engine-test GTest::GTest GTest::Main Threads::Threads)
target_link_libraries(order-test GTest::GTest GTest::Main Threads::Threads)

add_executable(order-book src/main.cpp)
target_link_libraries(order-book PRIVATE Threads::Threads)
