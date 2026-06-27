cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(layoutshadow VERSION 0.1.0)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON)
string(JSON MOD_ID GET "${MOD_JSON}" id)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ShadowManager.cpp
    src/Spout.cpp
    src/LayoutGate.cpp
    src/layout_mode.cpp
    src/Overlay.cpp
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# layout_mode.cpp holds two giant ID tables; /bigobj keeps the TU compilable.
# CI uses clang-cl (MSVC ABI), so match on Windows rather than the MSVC generator.
if (WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()
