add_executable(order-book src/main.cpp)
target_include_directories(order-book PRIVATE include)
target_link_libraries(order-book PRIVATE 
    Threads::Threads 
    common
)

add_executable(engine-test test/engine_test.cpp) 
target_include_directories(engine-test PRIVATE include)
target_compile_definitions(engine-test PRIVATE UNIT_TEST) 
target_link_libraries(engine-test PRIVATE 
    GTest::gtest_main 
    Threads::Threads
    common
)
gtest_discover_tests(engine-test)

add_executable(order-test test/order-book-test.cpp) 
target_include_directories(order-test PRIVATE include)
target_compile_definitions(order-test PRIVATE UNIT_TEST) 
target_link_libraries(order-test PRIVATE 
    GTest::gtest_main 
    Threads::Threads 
    common
)
gtest_discover_tests(order-test)

add_executable(micro-bench benchmark/micro_bench.cpp)
target_include_directories(micro-bench PRIVATE include)
target_link_libraries(micro-bench PRIVATE 
    benchmark::benchmark 
    Threads::Threads 
    common
)
