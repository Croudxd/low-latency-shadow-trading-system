add_executable(kdb src/main.cpp)
target_include_directories(kdb PUBLIC include)
target_link_libraries(kdb PRIVATE Threads::Threads common)

