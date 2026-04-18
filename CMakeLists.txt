cmake_minimum_required(VERSION 3.25)

project(rvcore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(rvcore ${SOURCES})

target_compile_options(rvcore PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wcast-qual
    -Wcast-align=strict
    -Wundef
    -Wconversion
    -Wsign-conversion
    -Wswitch-enum
    -Wimplicit-fallthrough=5
    -Wnull-dereference
    -Wdouble-promotion
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wold-style-cast
    -Wuseless-cast
    -Wzero-as-null-pointer-constant
    -O1
)