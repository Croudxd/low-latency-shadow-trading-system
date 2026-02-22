add_executable(kdb src/main.cpp)

target_include_directories(kdb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(kdb PRIVATE KXVER=3)

target_link_libraries(kdb 
    PRIVATE 
    common                              
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/c.o
    Threads::Threads                  
)
