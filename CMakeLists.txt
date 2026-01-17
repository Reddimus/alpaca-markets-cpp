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
set(ALPACA_MARKETS_BUILD_TESTS_DEFAULT ON)
set(ALPACA_MARKETS_BUILD_EXAMPLES_DEFAULT ON)
if(NOT PROJECT_IS_TOP_LEVEL)
    set(ALPACA_MARKETS_BUILD_TESTS_DEFAULT OFF)
    set(ALPACA_MARKETS_BUILD_EXAMPLES_DEFAULT OFF)
endif()

option(ALPACA_MARKETS_BUILD_TESTS "Build tests" ${ALPACA_MARKETS_BUILD_TESTS_DEFAULT})
option(ALPACA_MARKETS_BUILD_EXAMPLES "Build examples" ${ALPACA_MARKETS_BUILD_EXAMPLES_DEFAULT})
option(ALPACA_MARKETS_USE_SYSTEM_RAPIDJSON "Use system RapidJSON instead of FetchContent" OFF)
option(ALPACA_MARKETS_USE_SYSTEM_HTTPLIB "Use system cpp-httplib instead of FetchContent" OFF)

# Dependencies
include(FetchContent)

# RapidJSON (header-only)
if(ALPACA_MARKETS_USE_SYSTEM_RAPIDJSON)
    find_package(RapidJSON REQUIRED)
    set(alpaca_markets_rapidjson_include_dirs ${RapidJSON_INCLUDE_DIRS})
else()
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
    set(alpaca_markets_rapidjson_include_dirs ${rapidjson_SOURCE_DIR}/include)
endif()

# cpp-httplib (header-only)
if(ALPACA_MARKETS_USE_SYSTEM_HTTPLIB)
    find_package(httplib REQUIRED)
else()
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.14.3
    )
    FetchContent_MakeAvailable(httplib)
endif()

# Find OpenSSL for HTTPS support
find_package(OpenSSL REQUIRED)

# Find ZLIB for compression support (used by cpp-httplib)
find_package(ZLIB REQUIRED)

# Find Brotli for compression support (optional, used by cpp-httplib)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(BROTLI IMPORTED_TARGET libbrotlidec libbrotlienc)
endif()

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
    ${alpaca_markets_rapidjson_include_dirs}
)

# REST module
file(GLOB ALPACA_REST_SOURCES "src/rest/*.cpp")
add_library(alpaca_markets_rest OBJECT ${ALPACA_REST_SOURCES})
target_include_directories(alpaca_markets_rest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Mark RapidJSON as SYSTEM to suppress third-party deprecation warnings
target_include_directories(alpaca_markets_rest SYSTEM PRIVATE
    ${alpaca_markets_rapidjson_include_dirs}
)
target_link_libraries(alpaca_markets_rest PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(alpaca_markets_rest PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# Stream module
file(GLOB ALPACA_STREAM_SOURCES "src/stream/*.cpp")
add_library(alpaca_markets_stream OBJECT ${ALPACA_STREAM_SOURCES})
target_include_directories(alpaca_markets_stream PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Mark RapidJSON as SYSTEM to suppress third-party deprecation warnings
target_include_directories(alpaca_markets_stream SYSTEM PRIVATE
    ${alpaca_markets_rapidjson_include_dirs}
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

target_compile_features(alpaca_markets PUBLIC cxx_std_20)

# RapidJSON include directory (SYSTEM to suppress third-party warnings)
target_include_directories(alpaca_markets SYSTEM PRIVATE
    ${alpaca_markets_rapidjson_include_dirs}
)

# Link dependencies
target_link_libraries(alpaca_markets
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
)

# Optional Brotli support - use library names for export compatibility
if(BROTLI_FOUND)
    # Link directly to library files instead of PkgConfig target for export compatibility
    target_link_libraries(alpaca_markets PUBLIC ${BROTLI_LIBRARIES})
    target_include_directories(alpaca_markets SYSTEM PUBLIC ${BROTLI_INCLUDE_DIRS})
    target_compile_definitions(alpaca_markets PRIVATE CPPHTTPLIB_BROTLI_SUPPORT)
    set(ALPACA_MARKETS_BROTLI_ENABLED TRUE CACHE INTERNAL "Brotli support enabled")
endif()

# Enable OpenSSL support in cpp-httplib
target_compile_definitions(alpaca_markets PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# Create alias for namespaced usage
add_library(alpaca::markets ALIAS alpaca_markets)
set_target_properties(alpaca_markets PROPERTIES EXPORT_NAME markets)

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
include(CMakePackageConfigHelpers)

set(alpaca_markets_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/alpaca_markets")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/alpaca_markets-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/alpaca_markets-config.cmake"
    INSTALL_DESTINATION "${alpaca_markets_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/alpaca_markets-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS alpaca_markets
    EXPORT alpaca_markets_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

export(EXPORT alpaca_markets_targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/alpaca_markets-targets.cmake"
    NAMESPACE alpaca::
)

install(DIRECTORY include/alpaca
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT alpaca_markets_targets
    FILE alpaca_markets-targets.cmake
    NAMESPACE alpaca::
    DESTINATION ${alpaca_markets_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/alpaca_markets-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/alpaca_markets-config-version.cmake"
    DESTINATION ${alpaca_markets_INSTALL_CMAKEDIR}
)
