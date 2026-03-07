cmake_minimum_required(VERSION 3.16)
project(nbody_dynamics VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ═══════════════════════════════════════════════════════════════
#  Backend selection
#
#  GLFW  — default desktop, no multi-touch (mouse only)
#  SDL   — desktop with full multi-touch via SDL2 (laptop + RPi)
#  DRM   — headless kiosk, reads evdev directly  (RPi without DE)
# ═══════════════════════════════════════════════════════════════
set(BACKEND "GLFW" CACHE STRING "Windowing/input backend")
set_property(CACHE BACKEND PROPERTY STRINGS GLFW SDL DRM)

option(USE_GLES  "Force OpenGL ES 2.0 (auto-enabled for DRM)" OFF)
option(USE_WAYLAND "Hint SDL to prefer Wayland over X11/XWayland" ON)

# DRM always needs GLES
if(BACKEND STREQUAL "DRM")
    set(USE_GLES ON CACHE BOOL "" FORCE)
endif()

message(STATUS "")
message(STATUS "  Backend ........... ${BACKEND}")
message(STATUS "  OpenGL ES ......... ${USE_GLES}")
message(STATUS "  Prefer Wayland .... ${USE_WAYLAND}")
message(STATUS "")

# ═══════════════════════════════════════════════════════════════
#  raylib via FetchContent
# ═══════════════════════════════════════════════════════════════
include(FetchContent)

# Map our BACKEND to raylib's PLATFORM cache variable
if(BACKEND STREQUAL "SDL")
    set(PLATFORM "SDL" CACHE STRING "" FORCE)
elseif(BACKEND STREQUAL "DRM")
    set(PLATFORM "DRM" CACHE STRING "" FORCE)
else()
    set(PLATFORM "Desktop" CACHE STRING "" FORCE)
endif()

if(USE_GLES)
    set(OPENGL_VERSION "ES 2.0" CACHE STRING "" FORCE)
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(raylib)

# ═══════════════════════════════════════════════════════════════
#  Application target
# ═══════════════════════════════════════════════════════════════
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/assets.cpp
    src/input_handler.cpp
    src/renderer.cpp
    src/simulation.cpp
    src/star.cpp
    src/ui.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wno-missing-field-initializers
)

# ═══════════════════════════════════════════════════════════════
#  Backend-specific dependencies
#
#  raylib's own CMake *should* propagate these via PUBLIC link,
#  but FetchContent + static libs sometimes loses them.
#  Explicit is safer.
# ═══════════════════════════════════════════════════════════════
find_package(PkgConfig QUIET)

if(BACKEND STREQUAL "SDL")
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::SDL2)
    else()
        find_package(SDL2 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
    endif()

    # Tell SDL to use Wayland natively instead of XWayland.
    # This is a runtime env var — we bake it into a wrapper script
    # and also set it as a compile definition you can apply at startup.
    if(USE_WAYLAND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HINT_SDL_WAYLAND=1)
    endif()

elseif(BACKEND STREQUAL "DRM")
    if(NOT PkgConfig_FOUND)
        message(FATAL_ERROR "PkgConfig required for DRM backend")
    endif()
    pkg_check_modules(DRM_DEPS REQUIRED IMPORTED_TARGET
        libdrm gbm egl glesv2 libinput
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::DRM_DEPS)
endif()

# Common Unix libs
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl)
endif()

# ═══════════════════════════════════════════════════════════════
#  Copy assets beside the binary
# ═══════════════════════════════════════════════════════════════
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        COMMENT "Copying assets → build/assets"
    )
endif()

# ═══════════════════════════════════════════════════════════════
#  Generate a launch wrapper that sets SDL_VIDEODRIVER=wayland
#  so you don't have to remember the env var every time
# ═══════════════════════════════════════════════════════════════
if(BACKEND STREQUAL "SDL" AND USE_WAYLAND AND UNIX)
    set(WRAPPER_PATH "${CMAKE_BINARY_DIR}/run.sh")
    file(WRITE "${WRAPPER_PATH}"
"#!/bin/sh
# Auto-generated — launches with native Wayland touch support
export SDL_VIDEODRIVER=wayland
exec \"$(dirname \"$0\")/${PROJECT_NAME}\" \"$@\"
")
    execute_process(COMMAND chmod +x "${WRAPPER_PATH}")
    message(STATUS "Wrapper script: ${WRAPPER_PATH}")
endif()