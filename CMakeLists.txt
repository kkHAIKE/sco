cmake_minimum_required(VERSION 3.24)

project(cotest)

add_executable(cotest
    main.cpp
)

set_property(TARGET cotest PROPERTY CXX_STANDARD 20)
target_compile_options(cotest PRIVATE "-fcoroutines-ts")