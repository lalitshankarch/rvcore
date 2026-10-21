cmake_minimum_required(VERSION 3.25)

project(rvcore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(rvcore ${SOURCES})

find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
    include(FetchContent)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.18
    )

    FetchContent_MakeAvailable(SDL3)
endif()

target_link_libraries(rvcore PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_definitions(rvcore PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _HAS_EXCEPTIONS=0
    )
    target_compile_options(rvcore PRIVATE
        /W3
        /permissive-
        /EHs-c-
        /GR-
    )
else()
    target_compile_options(rvcore PRIVATE
        -Wall
        -Wextra
        -pedantic
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
        -fno-exceptions
        -fno-rtti
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(rvcore PRIVATE /O2)
    else()
        target_compile_options(rvcore PRIVATE -O3)
    endif()
endif()

include(CheckIPOSupported)

check_ipo_supported(RESULT ipo_supported)

if(ipo_supported)
    set_property(
        TARGET rvcore
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
endif()