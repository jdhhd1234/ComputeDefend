cmake_minimum_required(VERSION 3.24)
project(ComputerDefender VERSION 0.1.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# libcurl: fetched and built from source as a static library.
# TLS uses the native Windows TLS stack (Schannel), so no OpenSSL is needed.
# ---------------------------------------------------------------------------
include(FetchContent)

set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_MANPAGE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

set(CURL_ENABLE_SSL ON CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)

# Keep the build self-contained: skip optional third-party dependencies.
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)

FetchContent_Declare(curl
    URL https://curl.se/download/curl-8.21.0.tar.gz
    URL_HASH SHA256=D9B327997999045A24CDA50F3983E69E51C516BD8BE6EF9842FC7F99135E33BB
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(curl)

add_executable(ComputerDefender main.cpp
    main.hpp)
target_compile_features(ComputerDefender PRIVATE cxx_std_20)
set_target_properties(ComputerDefender PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(ComputerDefender PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ComputerDefender PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
    )
endif()

if(MINGW)
    # Avoid requiring MinGW runtime DLLs on the destination Windows machine.
    target_link_options(ComputerDefender PRIVATE -static)
endif()

target_link_libraries(ComputerDefender PRIVATE CURL::libcurl)
