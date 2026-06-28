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

# ============================================================================
# Spout2 — linked STATICALLY from vendored sources (no runtime DLL needed).
# We bypass Spout2's own root CMake (which pulls project()/runtime options that
# can collide with Geode's build) and compile the SpoutGL sources directly into
# a small static lib, then link it into the mod.
# ============================================================================
if (WIN32)
    set(SPOUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Spout2/SPOUTSDK/SpoutGL)

    add_library(Spout_static STATIC
        ${SPOUT_DIR}/Spout.cpp
        ${SPOUT_DIR}/SpoutCopy.cpp
        ${SPOUT_DIR}/SpoutDirectX.cpp
        ${SPOUT_DIR}/SpoutFrameCount.cpp
        ${SPOUT_DIR}/SpoutGL.cpp
        ${SPOUT_DIR}/SpoutGLextensions.cpp
        ${SPOUT_DIR}/SpoutReceiver.cpp
        ${SPOUT_DIR}/SpoutSender.cpp
        ${SPOUT_DIR}/SpoutSenderNames.cpp
        ${SPOUT_DIR}/SpoutSharedMemory.cpp
        ${SPOUT_DIR}/SpoutUtils.cpp
    )
    # No SPOUT_BUILD_DLL define -> SPOUT_DLLEXP expands to nothing (static build).
    target_include_directories(Spout_static PUBLIC ${SPOUT_DIR})
    # Spout needs the Windows system libs it lists in its own CMake.
    target_link_libraries(Spout_static PUBLIC
        opengl32 kernel32 user32 gdi32 winspool comdlg32 comctl32 advapi32
        shell32 ole32 oleaut32 uuid odbc32 odbccp32 d3d9 d3d11 dxgi version winmm
    )

    target_link_libraries(${PROJECT_NAME} Spout_static)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LAYOUT_SHADOW_SPOUT=1)
endif()

# layout_mode.cpp holds two giant ID tables; /bigobj keeps the TU compilable.
# Only MSVC and clang-cl understand the /-flag form. Plain clang++ (GCC driver,
# which some CI runners use to target Windows) would treat "/bigobj" as a file
# path and fail with "no such file or directory". Gate accordingly: emit /bigobj
# only for MSVC-compatible drivers. clang++ compiles these TUs fine at -O3
# without it (sections get folded below the COFF 65k limit).
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()
