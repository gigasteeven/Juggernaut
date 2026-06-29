cmake_minimum_required(VERSION 3.21)

project(gd_spout_mod VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON)
string(JSON MOD_ID GET "${MOD_JSON}" id)

# --- Spout2 Static Library ---
# Compiled WITHOUT Geode PCH to avoid glew.h redeclaration conflicts.
# GL_GLEXT_LEGACY must be the first define, before any GL headers.
add_library(Spout2 STATIC
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutSender.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutSenderNames.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutSharedMemory.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutDirectX.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutGL.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutCopy.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutUtils.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutFrameCount.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/SpoutGLextensions.cpp
    vendor/Spout2/SPOUTSDK/SpoutGL/Spout.cpp
)

# GL_GLEXT_LEGACY suppresses glew extension redeclarations
target_compile_definitions(Spout2 PRIVATE GL_GLEXT_LEGACY)
target_include_directories(Spout2 PUBLIC vendor/Spout2/SPOUTSDK/SpoutGL)

# DO NOT define NOGDI — Spout2 needs wingdi.h types
# Link Spout2 with necessary Windows libraries
if(WIN32)
    target_link_libraries(Spout2 opengl32 d3d11 dxgi version)
endif()

# --- Main Mod Target ---
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/layout_mode.cpp
)

# Setup Geode
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# Link Spout2 to main mod (plain signature — setup_geode_mod already used plain)
target_link_libraries(${PROJECT_NAME} Spout2)

# /bigobj only under MSVC; clang++ (GCC driver) doesn't understand /bigobj
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()
