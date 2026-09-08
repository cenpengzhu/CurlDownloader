set(TARGET_NAME CurlDownloader)

set(DEPENDENCY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependency")

# ---------------------------------------------------------------------------
# Vendored third-party dependencies (prebuilt static archives)
# ---------------------------------------------------------------------------
add_library(vendor::curl STATIC IMPORTED GLOBAL)
set_target_properties(vendor::curl PROPERTIES
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    IMPORTED_LOCATION "${DEPENDENCY_DIR}/curl/lib/libcurl_mt.lib"
    IMPORTED_LOCATION_RELEASE "${DEPENDENCY_DIR}/curl/lib/libcurl_mt.lib"
    IMPORTED_LOCATION_DEBUG "${DEPENDENCY_DIR}/curl/lib/libcurld_mt.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCY_DIR}/curl/include"
    INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB"
)
target_link_libraries(vendor::curl INTERFACE
    "${DEPENDENCY_DIR}/curl/lib/libssh2.lib"
    ws2_32
    wldap32
)

add_library(vendor::tinyxml2 STATIC IMPORTED GLOBAL)
set_target_properties(vendor::tinyxml2 PROPERTIES
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    IMPORTED_LOCATION "${DEPENDENCY_DIR}/tinyxml2/lib/tinyxml2_mt.lib"
    IMPORTED_LOCATION_RELEASE "${DEPENDENCY_DIR}/tinyxml2/lib/tinyxml2_mt.lib"
    IMPORTED_LOCATION_DEBUG "${DEPENDENCY_DIR}/tinyxml2/lib/tinyxml2d_mt.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCY_DIR}/tinyxml2/include"
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(CURLDOWNLOADER_CORE_SOURCES
    src/CDownloader.cpp
    src/CDownloadTaskManager.cpp
    src/CDownloadThread.cpp
    src/CLibcurlTool.cpp
    src/easylogging++.cc
    src/md5.cpp
    src/stdafx.cpp
)

set(CURLDOWNLOADER_API_SOURCES
    src/CurlDownloader.cpp
)

set(CURLDOWNLOADER_SOURCES
    ${CURLDOWNLOADER_CORE_SOURCES}
    ${CURLDOWNLOADER_API_SOURCES}
)

set(CURLDOWNLOADER_HEADERS
    src/CDownloader.h
    src/CDownloadInfo.h
    src/CDownloadTask.h
    src/CDownloadTaskManager.h
    src/CDownloadThread.h
    src/CDownloadThreadManager.h
    src/CLibcurlTool.h
    src/CTask.h
    src/CTaskManager.h
    src/CThread.h
    src/CThreadManager.h
    src/CurlDownloader.h
    src/easylogging++.h
    src/md5.h
    src/stdafx.h
)

# Object library shared between the DLL and test targets
add_library(${TARGET_NAME}_core OBJECT
    ${CURLDOWNLOADER_CORE_SOURCES}
    ${CURLDOWNLOADER_HEADERS}
)
source_group("Core Sources" FILES ${CURLDOWNLOADER_CORE_SOURCES})
source_group("Header Files" FILES ${CURLDOWNLOADER_HEADERS})

add_library(${TARGET_NAME} SHARED
    ${CURLDOWNLOADER_API_SOURCES}
    $<TARGET_OBJECTS:${TARGET_NAME}_core>
)
source_group("API Sources" FILES ${CURLDOWNLOADER_API_SOURCES})

# ---------------------------------------------------------------------------
# Usage requirements
# ---------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${DEPENDENCY_DIR}/tinyxml2/source"
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${DEPENDENCY_DIR}/tinyxml2/source"
)

target_compile_definitions(${TARGET_NAME}_core
    PRIVATE
        WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        DOWNLOADER_EXPORT
        _CRT_SECURE_NO_WARNINGS
        ELPP_STL_LOGGING
        ELPP_NO_DEFAULT_LOG_FILE
)

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        DOWNLOADER_EXPORT
        _CRT_SECURE_NO_WARNINGS
        ELPP_STL_LOGGING
        ELPP_NO_DEFAULT_LOG_FILE
)

target_link_libraries(${TARGET_NAME}_core
    PUBLIC
        vendor::curl
        vendor::tinyxml2
        Wininet
        winmm
        comctl32
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        ${TARGET_NAME}_core
)

if(MSVC)
    target_compile_options(${TARGET_NAME}_core PRIVATE /W3 /MP /utf-8)
    target_compile_options(${TARGET_NAME} PRIVATE /W3 /MP /utf-8)
    # stdafx.cpp re-implements __iob_func for the legacy archives built
    # against the pre-VS2015 CRT.
    target_link_libraries(${TARGET_NAME}_core PUBLIC legacy_stdio_definitions)
    target_link_options(${TARGET_NAME} PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
        $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES src/CurlDownloader.h DESTINATION include)
