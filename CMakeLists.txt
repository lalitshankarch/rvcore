cmake_minimum_required(VERSION 3.25)

project(rvcore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(rvcore ${SOURCES})

find_package(SDL3 REQUIRED)
target_link_libraries(rvcore PRIVATE SDL3::SDL3 mman)

target_compile_options(rvcore PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wformat=2
    -Wcast-qual
    -Wcast-align
    -Wundef
    -Wconversion
    -Wsign-conversion
    -Wswitch-enum
    -Wimplicit-fallthrough
    -Wnull-dereference
    -Wdouble-promotion
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wold-style-cast
    -Wzero-as-null-pointer-constant
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(rvcore PRIVATE -O3)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set_property(TARGET rvcore PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()