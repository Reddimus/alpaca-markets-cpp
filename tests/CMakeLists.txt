# Tests CMakeLists.txt

file(GLOB TEST_SOURCES "*.cpp")

add_executable(alpaca_markets_tests ${TEST_SOURCES})

target_link_libraries(alpaca_markets_tests
    PRIVATE
        alpaca_markets
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(alpaca_markets_tests PRIVATE
    ${rapidjson_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(alpaca_markets_tests)
