cmake_minimum_required(VERSION 3.25)
project(alpaca_markets VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard (widely supported; update to C++23/26 when compilers support it)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings for our code (strict)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Options
option(ALPACA_MARKETS_BUILD_TESTS "Build tests" ON)
option(ALPACA_MARKETS_BUILD_EXAMPLES "Build examples" ON)

# Dependencies via FetchContent
include(FetchContent)

# RapidJSON (header-only, only need to populate)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
)
FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
    # Don't add_subdirectory - rapidjson is header-only and its CMakeLists brings in gtest
endif()

# cpp-httplib (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

# Find OpenSSL for HTTPS support
find_package(OpenSSL REQUIRED)

# ============================================================================
# Module targets (for selective builds)
# ============================================================================

# Models module
file(GLOB ALPACA_MODELS_SOURCES "src/models/*.cpp")
add_library(alpaca_markets_models OBJECT ${ALPACA_MODELS_SOURCES})
target_include_directories(alpaca_markets_models PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Mark RapidJSON as SYSTEM to suppress third-party deprecation warnings
target_include_directories(alpaca_markets_models SYSTEM PRIVATE
    ${rapidjson_SOURCE_DIR}/include
)

# REST module
file(GLOB ALPACA_REST_SOURCES "src/rest/*.cpp")
add_library(alpaca_markets_rest OBJECT ${ALPACA_REST_SOURCES})
target_include_directories(alpaca_markets_rest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Mark RapidJSON as SYSTEM to suppress third-party deprecation warnings
target_include_directories(alpaca_markets_rest SYSTEM PRIVATE
    ${rapidjson_SOURCE_DIR}/include
)
target_link_libraries(alpaca_markets_rest PUBLIC httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(alpaca_markets_rest PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

# Stream module
file(GLOB ALPACA_STREAM_SOURCES "src/stream/*.cpp")
add_library(alpaca_markets_stream OBJECT ${ALPACA_STREAM_SOURCES})
target_include_directories(alpaca_markets_stream PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Mark RapidJSON as SYSTEM to suppress third-party deprecation warnings
target_include_directories(alpaca_markets_stream SYSTEM PRIVATE
    ${rapidjson_SOURCE_DIR}/include
)

# ============================================================================
# Main library (combines all modules)
# ============================================================================
add_library(alpaca_markets
    $<TARGET_OBJECTS:alpaca_markets_models>
    $<TARGET_OBJECTS:alpaca_markets_rest>
    $<TARGET_OBJECTS:alpaca_markets_stream>
)

# Define include directories
target_include_directories(alpaca_markets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# RapidJSON include directory (SYSTEM to suppress third-party warnings)
target_include_directories(alpaca_markets SYSTEM PRIVATE
    ${rapidjson_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(alpaca_markets
    PUBLIC
        httplib::httplib
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Enable OpenSSL support in cpp-httplib
target_compile_definitions(alpaca_markets PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

# Create alias for namespaced usage
add_library(alpaca::markets ALIAS alpaca_markets)

# Tests
if(ALPACA_MARKETS_BUILD_TESTS)
    enable_testing()
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Suppress warnings for third-party googletest/googlemock targets
    if(TARGET gtest)
        target_compile_options(gtest PRIVATE -w)
    endif()
    if(TARGET gtest_main)
        target_compile_options(gtest_main PRIVATE -w)
    endif()
    if(TARGET gmock)
        target_compile_options(gmock PRIVATE -w)
    endif()
    if(TARGET gmock_main)
        target_compile_options(gmock_main PRIVATE -w)
    endif()
    
    add_subdirectory(tests)
endif()

# Examples
if(ALPACA_MARKETS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS alpaca_markets
    EXPORT alpaca_markets_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/alpaca
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT alpaca_markets_targets
    FILE alpaca_markets-targets.cmake
    NAMESPACE alpaca::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alpaca_markets
)
