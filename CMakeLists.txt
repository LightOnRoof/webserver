cmake_minimum_required(VERSION 3.14)
project(uWS_Server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find Dependencies
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# 2. Fetch uWebSockets
include(FetchContent)
FetchContent_Declare(
    uwebsockets
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG v20.70.0
)
FetchContent_MakeAvailable(uwebsockets)

# 3. Define the Executable
add_executable(server src/main.cpp)

# 4. Correctly Collect ALL uSockets source files (Recursive)
# uSockets core + event loops + crypto
file(GLOB_RECURSE USOCKETS_SOURCES 
    "${uwebsockets_SOURCE_DIR}/uSockets/src/*.c"
    "${uwebsockets_SOURCE_DIR}/uSockets/src/*.cpp"
)
target_sources(server PRIVATE ${USOCKETS_SOURCES})

# 5. Include Paths
target_include_directories(server PRIVATE 
    ${uwebsockets_SOURCE_DIR}/src
    ${uwebsockets_SOURCE_DIR}/uSockets/src
)

# 6. Critical Compile Definitions
# This tells uSockets to use OpenSSL and the default event loop (epoll/kqueue)
target_compile_definitions(server PRIVATE LIBUS_USE_OPENSSL)

# 7. Link Libraries
target_link_libraries(server PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    ZLIB::ZLIB
    Threads::Threads
)

# 8. Platform specific adjustments
if(APPLE)
    # macOS requires corefoundation for kqueue
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(server PRIVATE ${CORE_FOUNDATION})
endif()


target_compile_options(server PRIVATE -pthread)