# cmake_minimum_required(VERSION 3.10)
# project(order-book)
#
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_STANDARD 20)
#
# include_directories(include)

# enable_testing()

# add_executable(order-book src/main.cpp)
#add_executable(order-book-test test/test_.cpp)
# add_executable(order-book-test test/test.cpp)
# add_executable(order-book-benchmark test/benchmark.cpp)
#

#TEST:
# add_test(NAME test COMMAND order-book-test)
cmake_minimum_required(VERSION 3.10)
project(order-book)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Optimization Flags
# -O3: Maximum optimization level
# -march=native: Tune code for your specific CPU (AVX, BMI2, etc.)
# -flto: Link-time optimization (massive for inlining across files)
# -fno-stack-protector: (Optional) Removes stack checks for a tiny speed boost
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")

include_directories(include)

# Main Executable
add_executable(order-book src/main.cpp)

add_executable(order-book-test test/test.cpp)
# Benchmark Executable
add_executable(order-book-benchmark test/benchmark.cpp)

# Link threading library (required for pthread/std::thread)
find_package(Threads REQUIRED)
target_link_libraries(order-book-benchmark PRIVATE Threads::Threads)
