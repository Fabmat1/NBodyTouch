cmake_minimum_required(VERSION 3.25)
project(NBodyTouch)

set(CMAKE_CXX_STANDARD 17)

set(NBODY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")
set(RAYLIB_ROOT "${NBODY_ROOT}/../raylib")

if(NOT EXISTS "${RAYLIB_ROOT}/src/rcore.c")
    message(FATAL_ERROR "raylib not found at ${RAYLIB_ROOT}")
endif()

# Build raylib via its own CMake
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(OPENGL_VERSION "ES 2.0" CACHE STRING "" FORCE)
set(PLATFORM "Android" CACHE STRING "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
add_subdirectory("${RAYLIB_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/raylib_build")

add_library(NBodyTouch SHARED
    ${NBODY_ROOT}/src/main.cpp
    ${NBODY_ROOT}/src/app.cpp
    ${NBODY_ROOT}/src/simulation.cpp
    ${NBODY_ROOT}/src/star.cpp
    ${NBODY_ROOT}/src/renderer.cpp
    ${NBODY_ROOT}/src/input_handler.cpp
    ${NBODY_ROOT}/src/ui.cpp
    ${NBODY_ROOT}/src/assets.cpp
)

target_include_directories(NBodyTouch PRIVATE
    ${NBODY_ROOT}/include
    ${RAYLIB_ROOT}/src
)

target_compile_definitions(NBodyTouch PRIVATE
    PLATFORM_ANDROID
)

find_library(ANDROID_LOG log)
find_library(ANDROID_LIB android)
find_library(GLES_LIB GLESv2)
find_library(EGL_LIB EGL)

# Force-link all raylib symbols so ANativeActivity_onCreate isn't stripped
target_link_libraries(NBodyTouch
    -Wl,--whole-archive
    raylib
    -Wl,--no-whole-archive
    ${ANDROID_LOG}
    ${ANDROID_LIB}
    ${GLES_LIB}
    ${EGL_LIB}
    m
)
