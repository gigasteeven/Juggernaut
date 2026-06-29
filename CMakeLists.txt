cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(spoutshadow VERSION 0.1.0)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON)
string(JSON MOD_ID GET "${MOD_JSON}" id)

if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK env var not set")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# === Spout2 как изолированная статическая библиотека ===
# Без Geode PCH, GL_GLEXT_LEGACY первым — глушит redeclaration Glew vs Spout.
# БЕЗ NOGDI — Spout2 нуждается в wingdi.h типах.
set(SPOUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/Spout2/SPOUTSDK/SpoutGL")
set(SPOUT_SOURCES
    ${SPOUT_DIR}/Spout.cpp
    ${SPOUT_DIR}/SpoutGL.cpp
    ${SPOUT_DIR}/SpoutCopy.cpp
    ${SPOUT_DIR}/SpoutDirectX.cpp
    ${SPOUT_DIR}/SpoutFrameCount.cpp
    ${SPOUT_DIR}/SpoutGLextensions.cpp
    ${SPOUT_DIR}/SpoutSenderNames.cpp
)
add_library(spout2_static STATIC ${SPOUT_SOURCES})
target_include_directories(spout2_static PUBLIC ${SPOUT_DIR})
target_compile_definitions(spout2_static PRIVATE
    GL_GLEXT_LEGACY        # ПЕРВЫМ — критичный нюанс №1
    SPOUT_STATIC_DEFINE    # статическая линковка, никакой runtime DLL
)
# Geode PCH НЕ подключаем — именно тут лежит конфликт glew.h vs Spout.
if(MSVC)
    target_compile_options(spout2_static PRIVATE /bigobj)
endif()

# === Главный таргет мода ===
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ShadowManager.cpp
    src/SpoutManager.cpp
    vendor/solid_ids.cpp
    vendor/layout_mode.cpp
    src/hooks/PlayLayerHooks.cpp
    src/hooks/FMODHooks.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

setup_geode_mod(${PROJECT_NAME})

# Plain-сигнатура (без PRIVATE) — setup_geode_mod уже использовал plain,
# смешивание plain/keyword = ошибка CMake (критичный нюанс №3).
target_link_libraries(${PROJECT_NAME} spout2_static)

if(MSVC)
    # /bigobj только под MSVC — clang++ (GCC driver) падает на /bigobj (нюанс №4)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()