cmake_minimum_required(VERSION 3.20)
project(hdr_class VERSION 1.0 LANGUAGES CXX)

set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE x86)

if(MSVC)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    
    add_compile_options(/W4 /WX /MP)
    add_compile_options(/permissive-)
    add_compile_options(/utf-8)
    add_compile_options(/Zc:__cplusplus)
    
    add_compile_options(/arch:IA32)
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    
    add_definitions(-D_MBCS)
    add_definitions(-DUNICODE=0)
    add_definitions(-D_UNICODE=0)
    add_compile_options(/D_MBCS)
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/hdr.cpp
)

set(HEADERS
    include/hdr.h
)

add_executable(hdr_class ${SOURCES} ${HEADERS})

if(MSVC)
    set_target_properties(hdr_class PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/release"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/debug"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/release"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/debug"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/release"
    )
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT hdr_class)