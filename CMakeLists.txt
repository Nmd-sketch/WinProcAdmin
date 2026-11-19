cmake_minimum_required(VERSION 3.28)

project(WinProcAdmin
    VERSION 0.1.0
    DESCRIPTION "Win32 process administration tool with a Raylib/Raygui GUI"
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json so editors (clangd, etc.) can resolve raylib.h
# and raygui.h from the fetched dependency include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Core Win32 calls need no extra libs under MinGW; these are the ones the tool
# will commonly reach for as it grows.
set(WIN32_SYSTEM_LIBS kernel32 user32 gdi32 advapi32 shell32 comctl32)

# ---------------------------------------------------------------------------
# Dependencies (fetched and built from source with the active MinGW toolchain)
# ---------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# Raygui has no root CMakeLists.txt; its CMake project lives in projects/CMake.
FetchContent_Declare(raygui
    URL https://codeload.github.com/raysan5/raygui/tar.gz/refs/tags/5.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR projects/CMake
)
set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raygui)

# Raygui is a single-header library: exactly one translation unit must define
# RAYGUI_IMPLEMENTATION. Generate it in the build tree to keep sources clean.
set(RAYGUI_IMPL_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(RAYGUI_IMPL_SOURCE "${RAYGUI_IMPL_DIR}/raygui_impl.c")
file(MAKE_DIRECTORY "${RAYGUI_IMPL_DIR}")
file(WRITE "${RAYGUI_IMPL_SOURCE}"
"#define RAYGUI_IMPLEMENTATION\n#include \"raygui.h\"\n")
add_library(raygui_impl STATIC "${RAYGUI_IMPL_SOURCE}")
target_link_libraries(raygui_impl PUBLIC raygui raylib)

# GoogleTest
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.17.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
add_executable(WinProcAdmin src/main.cpp src/ProcessMonitor.cpp src/gui/gui.cpp src/ProcessQueue.cpp src/ProcSrwLock.cpp)
target_link_libraries(WinProcAdmin PRIVATE raylib raygui_impl ${WIN32_SYSTEM_LIBS})
target_compile_definitions(WinProcAdmin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

# CMake writes the database into the build tree; mirror it at the project root.
add_custom_target(export_compile_commands ALL
    COMMAND ${CMAKE_COMMAND}
            "-DDB_INPUT=${CMAKE_BINARY_DIR}/compile_commands.json"
            "-DDB_OUTPUT=${CMAKE_SOURCE_DIR}/compile_commands.json"
            "-DDB_BUILD_DIR=${CMAKE_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExportCompileCommands.cmake"
    DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
    COMMENT "Exporting compile_commands.json to project root"
    VERBATIM)
