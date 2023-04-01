cmake_minimum_required(VERSION 3.12)

project(cotest)

add_executable(cotest
    main.cpp
)

set_property(TARGET cotest PROPERTY CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(cotest PRIVATE "-fcoroutines-ts")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_package(Threads)
    target_compile_options(cotest PRIVATE "-fcoroutines")
    target_link_libraries(cotest ${CMAKE_THREAD_LIBS_INIT})
endif()
