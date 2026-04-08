cmake_minimum_required(VERSION 3.14)
project(uWS_Server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# nlohmann/json via FetchContent
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# ------------------------------------------------------------------
# Paths to bundled uWebSockets / uSockets
# ------------------------------------------------------------------
set(UWS_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets")
set(USOCKETS_DIR "${UWS_DIR}/uSockets")

# Collect uSockets sources (C and C++)
file(GLOB_RECURSE USOCKETS_SOURCES
    "${USOCKETS_DIR}/src/*.c"
    "${USOCKETS_DIR}/src/*.cpp"
)

# ------------------------------------------------------------------
# Main executable
# ------------------------------------------------------------------
add_executable(server src/main.cpp ${USOCKETS_SOURCES})

# Include directories
target_include_directories(server PRIVATE
    "${UWS_DIR}/src"
    "${USOCKETS_DIR}/src"
)

# Compile definitions
target_compile_definitions(server PRIVATE
    LIBUS_USE_OPENSSL
)

# Linking
target_link_libraries(server PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    Threads::Threads
)

# Platform-specific linking
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(CORE_FOUNDATION)
        target_link_libraries(server PRIVATE ${CORE_FOUNDATION})
    endif()
elseif(UNIX AND NOT APPLE)
    # stdc++fs is usually not needed with C++20 and recent compilers,
    # but kept for older toolchains if you use <filesystem>
    # target_link_libraries(server PRIVATE stdc++fs)
endif()

# Copy public assets to build directory (at configure time)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src/public"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")