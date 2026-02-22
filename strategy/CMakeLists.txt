add_executable(strategy src/main.cpp)
target_include_directories(strategy PUBLIC include)
target_link_libraries(strategy PRIVATE Threads::Threads common)

add_executable(engine_test test/test_engine.cpp) 
target_include_directories(engine_test PUBLIC include)
target_compile_definitions(engine_test PRIVATE UNIT_TEST) 
target_link_libraries(engine_test PRIVATE 
    GTest::gtest_main 
    Threads::Threads 
    common
)
gtest_discover_tests(engine_test)

# add_executable(micro-bench benchmark/micro_bench.cpp)
# target_link_libraries(micro-bench PRIVATE 
#     benchmark::benchmark 
#     Threads::Threads 
# )
