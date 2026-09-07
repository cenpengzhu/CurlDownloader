cmake_minimum_required(VERSION 3.15)

project(CurlDownloader
    VERSION 1.0.0
    DESCRIPTION "A multithread downloader component based on libcurl on Windows"
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "CurlDownloader is a Windows-only component.")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Compilation database (compile_commands.json) for clangd / clang-tidy / IDEs
# ---------------------------------------------------------------------------
option(CURLDOWNLOADER_EXPORT_COMPILE_COMMANDS
    "Generate compile_commands.json and copy it to the source tree" ON)

if(CURLDOWNLOADER_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    if(NOT CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
        message(WARNING
            "CMAKE_EXPORT_COMPILE_COMMANDS is only supported by the Ninja and "
            "Makefile generators; the current generator is '${CMAKE_GENERATOR}'. "
            "Configure with -G Ninja to obtain compile_commands.json.")
    endif()
endif()

# The vendored libcurl / tinyxml2 archives (*_mt.lib) are linked against the
# static multithreaded CRT, so the component must use it as well.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(CurlDownloader/CurlDownloader)

# Mirror the generated database next to the sources so that tooling launched
# from the project root picks it up without extra configuration.
if(CURLDOWNLOADER_EXPORT_COMPILE_COMMANDS AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    add_custom_target(copy_compile_commands ALL
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${CMAKE_SOURCE_DIR}/compile_commands.json"
        DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json to the source tree"
        VERBATIM
    )
endif()
