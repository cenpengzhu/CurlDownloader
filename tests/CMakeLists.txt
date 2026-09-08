include(FetchContent)

# ---------------------------------------------------------------------------
# GoogleTest
#
# Pinned to v1.14.0: the last release that still supports C++14, which is the
# standard this project targets. Newer releases require C++17.
# ---------------------------------------------------------------------------
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

# GoogleTest emits a lot of noise under /W3 that is not actionable here.
if(MSVC)
    foreach(gtest_target gtest gtest_main gmock gmock_main)
        if(TARGET ${gtest_target})
            target_compile_options(${gtest_target} PRIVATE /W0 /utf-8)
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
set(TEST_TARGET CurlDownloaderTests)

set(CURLDOWNLOADER_TEST_SOURCES
    test_main.cpp
    test_ctask.cpp
    test_ctask_manager.cpp
    test_cdownload_info.cpp
    test_cdownload_task_manager.cpp
    test_cthread_manager.cpp
    test_libcurl_tool.cpp
)

add_executable(${TEST_TARGET}
    ${CURLDOWNLOADER_TEST_SOURCES}
)

# Linking the OBJECT library pulls in both its object files and its usage
# requirements (include dirs, vendored curl/tinyxml2, system libs).
target_link_libraries(${TEST_TARGET}
    PRIVATE
        CurlDownloader_core
        GTest::gtest
)

target_compile_definitions(${TEST_TARGET}
    PRIVATE
        WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        DOWNLOADER_STATIC
        _CRT_SECURE_NO_WARNINGS
        ELPP_STL_LOGGING
        ELPP_NO_DEFAULT_LOG_FILE
)

if(MSVC)
    target_compile_options(${TEST_TARGET} PRIVATE /W3 /MP /utf-8)
endif()

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET}
    DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 120
)
