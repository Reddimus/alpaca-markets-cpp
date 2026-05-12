cmake_minimum_required(VERSION 3.25)
project(alpaca_markets VERSION 0.1.0 LANGUAGES CXX)

# C++23 standard (matches sibling SDKs after 2026-05-11 Glaze migration).
set(CMAKE_CXX_STANDARD 23)
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
option(ALPACA_MARKETS_USE_SYSTEM_HTTPLIB "Use system cpp-httplib instead of FetchContent" OFF)

# Dependencies
include(FetchContent)

# Glaze - JSON library (compile-time reflection, ~4x parse speedup over the
# pre-2026-05-11 RapidJSON path on dense numeric payloads, which is the
# Market Data API v2 hot path). License: MIT.
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v7.6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

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

# Find Brotli for compression support (optional, used by cpp-httplib).
# pkg-config isn't available on Windows runners; on Windows we fall
# back to vcpkg's CMake config-mode for unofficial-brotli.
if(NOT WIN32)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(BROTLI IMPORTED_TARGET libbrotlidec libbrotlienc)
    endif()
else()
    find_package(unofficial-brotli CONFIG QUIET)
    if(unofficial-brotli_FOUND)
        # Build a compatible PkgConfig::BROTLI alias so the rest of
        # the CMakeLists doesn't have to branch on platform.
        add_library(PkgConfig::BROTLI INTERFACE IMPORTED)
        target_link_libraries(PkgConfig::BROTLI INTERFACE
            unofficial::brotli::brotlidec
            unofficial::brotli::brotlienc
            unofficial::brotli::brotlicommon
        )
        set(BROTLI_FOUND TRUE)
    endif()
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
target_link_libraries(alpaca_markets_models PRIVATE glaze::glaze)

# REST module
file(GLOB ALPACA_REST_SOURCES "src/rest/*.cpp")
add_library(alpaca_markets_rest OBJECT ${ALPACA_REST_SOURCES})
target_include_directories(alpaca_markets_rest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(alpaca_markets_rest PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto glaze::glaze)
target_compile_definitions(alpaca_markets_rest PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# Stream module
file(GLOB ALPACA_STREAM_SOURCES "src/stream/*.cpp")
add_library(alpaca_markets_stream OBJECT ${ALPACA_STREAM_SOURCES})
target_include_directories(alpaca_markets_stream PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(alpaca_markets_stream PRIVATE glaze::glaze)

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

target_compile_features(alpaca_markets PUBLIC cxx_std_23)

# Link dependencies
target_link_libraries(alpaca_markets
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
)

# Optional Brotli support. Link via the PkgConfig::BROTLI imported
# target - it propagates the right link line on every platform:
#   - Linux/macOS: pkg_check_modules populates link/include from .pc
#   - Windows/MSVC: our find_package(unofficial-brotli) wrapper above
#     builds the same INTERFACE target out of the vcpkg config-mode
#     unofficial::brotli::brotli{dec,enc,common} primitives.
# Using ${BROTLI_LIBRARIES} (the legacy variable) only worked on
# Linux because pkg_check_modules populated it; on Windows it was
# empty, and the alpaca_markets static archive's brotli symbols
# went unresolved at executable link time.
if(BROTLI_FOUND)
    target_link_libraries(alpaca_markets PUBLIC PkgConfig::BROTLI)
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
