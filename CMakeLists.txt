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

# === Spout2 — изолированная статическая библиотека ===
set(SPOUT_GL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/Spout2/SPOUTSDK/SpoutGL")

if(NOT EXISTS "${SPOUT_GL_DIR}/Spout.h")
    message(FATAL_ERROR
        "Spout2 sources not found at: ${SPOUT_GL_DIR}\n"
        "Fix options:\n"
        "  A) git submodule add https://github.com/leadedge/Spout2.git lib/Spout2\n"
        "     git commit -am 'Add Spout2 submodule' && git push\n"
        "  B) Скопировать SPOUTSDK/SpoutGL из leadedge/Spout2 в lib/Spout2/SPOUTSDK/SpoutGL\n"
        "     и закоммитить файлы напрямую."
    )
endif()

# Собираем все .cpp из SpoutGL — надёжнее хардкода, ловит SpoutSharedMemory/SpoutUtils тоже.
file(GLOB SPOUT_SOURCES "${SPOUT_GL_DIR}/*.cpp")

add_library(spout2_static STATIC ${SPOUT_SOURCES})
target_include_directories(spout2_static PUBLIC ${SPOUT_GL_DIR})
target_compile_definitions(spout2_static PRIVATE
    GL_GLEXT_LEGACY        # ПЕРВЫМ — глушит redeclaration Glew vs Spout
    SPOUT_STATIC_DEFINE    # статическая линковка, никакой runtime DLL
)
# Geode PCH НЕ подключаем — именно тут лежит конфликт glew.h vs Spout.
if(MSVC)
    target_compile_options(spout2_static PRIVATE /bigobj)
endif()

# === Главный таргет мода ===
set(MOD_SOURCES
    src/main.cpp
    src/ShadowManager.cpp
    src/SpoutManager.cpp
    src/LayoutGate.cpp
    src/hooks/PlayLayerHooks.cpp
    src/hooks/FMODHooks.cpp
    vendor/solid_ids.cpp
    vendor/layout_mode.cpp
)

# Проверка существования всех исходников — даёт понятную ошибку, а не "No SOURCES"
foreach(src ${MOD_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        message(FATAL_ERROR "Missing source file: ${src}\nCreate it and commit.")
    endif()
endforeach()

add_library(${PROJECT_NAME} SHARED ${MOD_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

setup_geode_mod(${PROJECT_NAME})

# Plain-сигнатура (без PRIVATE) — setup_geode_mod уже использовал plain.
target_link_libraries(${PROJECT_NAME} spout2_static)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()