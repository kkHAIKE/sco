cmake_minimum_required(VERSION 3.14)
project(sco_httpcache CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# global
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include(FetchContent)

# sco
if (NOT TARGET sco::sco)
    FetchContent_Declare(
        sco
        GIT_REPOSITORY https://github.com/kkHAIKE/sco.git
        GIT_TAG        main
    )
    FetchContent_MakeAvailable(sco)
endif()

# libhv
FetchContent_Declare(
    hv
    URL https://github.com/ithewei/libhv/archive/refs/tags/v1.3.0.tar.gz
    URL_HASH SHA256=4dc3e5ae851e53773cd95b841dd240d02f8ea490d6df24ba198bcb27cacd5283
)

include(ExternalProject)
include(build_external_project)

# hiredis
build_external_project(hiredis_DIST
    hiredis
    https://github.com/redis/hiredis/archive/refs/tags/v1.1.0.tar.gz
    fe6d21741ec7f3fc9df409d921f47dfc73a4d8ff64f4ac6f1d95f951bf7f53d6
    -D BUILD_SHARED_LIBS=OFF
)
find_package(hiredis REQUIRED PATHS "${hiredis_DIST}" NO_DEFAULT_PATH)

# libuv
build_external_project(uv_DIST
    uv
    https://github.com/libuv/libuv/archive/refs/tags/v1.44.2.tar.gz
    e6e2ba8b4c349a4182a33370bb9be5e23c51b32efb9b9e209d0e8556b73a48da
    # -D LIBUV_BUILD_SHARED=OFF
    -D BUILD_TESTING=OFF
)
find_package(libuv REQUIRED PATHS "${uv_DIST}" NO_DEFAULT_PATH)
# FILE(COPY "${uv_DIST}/lib/libuv_a.a" DESTINATION "${uv_DIST}/lib/libuv.a")

# redis++
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_CXX_STANDARD 20 CACHE STRING "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_ASYNC libuv CACHE STRING "" FORCE)
# set(REDIS_PLUS_PLUS_BUILD_CORO ON CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_ASYNC_LIB_HEADER ${uv_DIST}/include CACHE PATH "" FORCE)
set(REDIS_PLUS_PLUS_ASYNC_LIB ${uv_DIST}/lib CACHE PATH "" FORCE)
FetchContent_Declare(
    redispp
    URL https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.7.tar.gz
    URL_HASH SHA256=89cb83b0a23ac5825300c301814eab74aa3cdcfcd12e87d443c2692e367768ba
)

FetchContent_MakeAvailable(hv redispp)

add_executable(httpcache main.cpp)

target_include_directories(httpcache PRIVATE ${redispp_SOURCE_DIR}/src)
target_link_libraries(httpcache PRIVATE sco::sco hv_static redis++::redis++_static uv_a)
