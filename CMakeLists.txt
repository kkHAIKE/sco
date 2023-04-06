cmake_minimum_required(VERSION 3.12)

project(sco CXX)

include(GNUInstallDirs)

# global
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# current repo check
if(NOT DEFINED SCO_MASTER_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(SCO_MASTER_PROJECT ON)
    else()
        set(SCO_MASTER_PROJECT OFF)
    endif()
endif()

# options
option(SCO_BUILD_SHARED "Build shared library" OFF)
option(SCO_BUILD_EXAMPLE "Build example" ${SCO_MASTER_PROJECT})

# source code
file(GLOB SCO_ALL_HEADERS "include/*.h" "include/*.hpp")
SET(SCO_SRCS "src/sco.cpp")

# library
if(SCO_BUILD_SHARED OR BUILD_SHARED_LIBS)
    add_library(sco SHARED ${SCO_SRCS} ${SCO_ALL_HEADERS})
    target_compile_definitions(sco PUBLIC SCO_SHARED_LIB)
else()
    add_library(sco STATIC ${SCO_SRCS} ${SCO_ALL_HEADERS})
endif()

add_library(sco::sco ALIAS sco)

target_compile_definitions(sco PUBLIC SCO_COMPILED_LIB)
target_include_directories(sco PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# header only
add_library(sco_header_only INTERFACE)
add_library(sco::sco_header_only ALIAS sco_header_only)

target_include_directories(sco_header_only INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                                     "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
# compile
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sco PUBLIC "-fcoroutines-ts")
    target_compile_options(sco_header_only INTERFACE "-fcoroutines-ts")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(sco PUBLIC "-fcoroutines")
    target_compile_options(sco_header_only INTERFACE "-fcoroutines")
endif()

# example
if (SCO_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
