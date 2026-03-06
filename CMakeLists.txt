cmake_minimum_required(VERSION 3.6)
project(NBodyApp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Options ─────────────────────────────────────────────────────
option(USE_SYSTEM_RAYLIB "Use system-installed raylib instead of FetchContent" OFF)
option(RPI_BUILD         "Build targeting Raspberry Pi 5 (aarch64)"           OFF)

# ── Raylib dependency ───────────────────────────────────────────
if(USE_SYSTEM_RAYLIB)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
    set(RAYLIB_TARGET PkgConfig::RAYLIB)
    message(STATUS "Using system raylib: ${RAYLIB_LINK_LIBRARIES}")
else()
    include(FetchContent)
    message(STATUS "Fetching raylib from GitHub (this may take a moment)...")
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    if(RPI_BUILD)
        set(OPENGL_VERSION "ES 3.0" CACHE STRING "" FORCE)
        set(WITH_PIC ON CACHE BOOL "" FORCE)
    endif()
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
    FetchContent_MakeAvailable(raylib)
    set(RAYLIB_TARGET raylib)
endif()

# ── Pi-specific flags ──────────────────────────────────────────
if(RPI_BUILD)
    add_compile_definitions(PLATFORM_RPI=1)
    # Link EGL/GLESv2 explicitly if needed
    find_library(EGL_LIB EGL)
    find_library(GLES_LIB GLESv2)
endif()

# ── Sources ─────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/star.cpp
    src/simulation.cpp
    src/renderer.cpp
    src/input_handler.cpp
    src/ui.cpp
    src/assets.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_TARGET})

if(RPI_BUILD AND EGL_LIB AND GLES_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${EGL_LIB} ${GLES_LIB})
endif()

# ── Copy assets ─────────────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)