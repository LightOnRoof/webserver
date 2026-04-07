cmake_minimum_required(VERSION 3.14)
project(uWS_Server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define paths for the bundled source
set(UWS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets")
set(USOCKETS_DIR "${UWS_DIR}/uSockets")
include(FetchContent)
# 2. Find system-level dependencies (still needed for SSL/Zlib)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# 3. Create the executable
add_executable(server src/main.cpp)

# 4. Collect ALL uSockets source files from the third_party folder
file(GLOB_RECURSE USOCKETS_SOURCES 
"${USOCKETS_DIR}/src/*.c"
"${USOCKETS_DIR}/src/*.cpp"
)
target_sources(server PRIVATE ${USOCKETS_SOURCES})

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Link it to your server
target_link_libraries(server PRIVATE nlohmann_json::nlohmann_json)

# 5. Include paths pointing to third_party
target_include_directories(server PRIVATE 
"${UWS_DIR}/src"
    "${USOCKETS_DIR}/src"
)

# 6. Compile definitions for uSockets
target_compile_definitions(server PRIVATE LIBUS_USE_OPENSSL)

# 7. Link system libraries
target_link_libraries(server PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    ZLIB::ZLIB
    Threads::Threads
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/public DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 8. Platform specific logic
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(server PRIVATE ${CORE_FOUNDATION})
elseif(UNIX)
    # On some Linux systems, you might need the filesystem library
    target_link_libraries(server PRIVATE stdc++fs)
endif()