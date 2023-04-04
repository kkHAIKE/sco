cmake_minimum_required(VERSION 3.12)
project(sco_examples CXX)

add_executable(example example.cpp)

find_package(Threads)
target_link_libraries(example PRIVATE sco::sco ${CMAKE_THREAD_LIBS_INIT})
