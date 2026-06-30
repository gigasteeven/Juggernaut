cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Juggernaut VERSION 1.0.0)

# ─── Spout2 wrapper: separate static lib, NO Geode PCH ───
# GL_GLEXT_LEGACY prevents Spout2 headers from redefining GL extension
# typedefs that conflict with Geode's glew.h brought in via PCH.
file(GLOB SPOUT_SOURCES "src/spout/SpoutGL/*.cpp")
add_library(spout_wrapper STATIC
    src/spout/spout_wrapper.cpp
    ${SPOUT_SOURCES}
)
target_include_directories(spout_wrapper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spout/SpoutGL
)
target_compile_definitions(spout_wrapper PRIVATE
    GL_GLEXT_LEGACY
    SPOUT_BUILD_DLL=0
)
# Do NOT set precompiled header for spout_wrapper — it must NOT see Geode PCH
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(spout_wrapper PRIVATE -mssse3)
elseif(MSVC)
    target_compile_options(spout_wrapper PRIVATE /bigobj /arch:AVX)
endif()
# Link OpenGL + d3d11 needed by Spout internals
target_link_libraries(spout_wrapper PRIVATE opengl32 d3d11 dxgi shlwapi)

# ─── Main mod target ───
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/shadow_layer.cpp
    src/layout_mode.cpp
    src/spout_output.cpp
    src/audio_guard.cpp
    src/indicators.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Plain signature — setup_geode_mod already used plain, mixing PRIVATE = error
target_link_libraries(${PROJECT_NAME} spout_wrapper opengl32)

# ─── Geode SDK ───
if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "GEODE_SDK env var undefined — set it or use Geode CLI"
    )
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
