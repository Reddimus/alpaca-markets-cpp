# Tests CMakeLists.txt

file(GLOB TEST_SOURCES "*_test.cpp")

add_executable(alpaca_markets_tests ${TEST_SOURCES})

target_link_libraries(alpaca_markets_tests
    PRIVATE
        alpaca_markets
        glaze::glaze
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(alpaca_markets_tests)

# Glaze parse-throughput regression guard. The bench parses a representative
# Market Data API v2 bars-by-symbol payload 1k times and asserts us/op stays
# under a cap. ctest --timeout = 30s; actual wall-clock is ~50ms on
# x86_64-v3, so the timeout fires only on catastrophic regression. The
# benchmark binary is built separately so it doesn't pull GoogleTest in.
add_executable(alpaca_markets_parse_benchmark
    parse_benchmark.cpp
)
target_link_libraries(alpaca_markets_parse_benchmark PRIVATE
    alpaca_markets
)
if(ALPACA_MARKETS_ENABLE_SANITIZERS)
    add_test(NAME alpaca_markets_parse_benchmark COMMAND alpaca_markets_parse_benchmark --no-timing)
else()
    add_test(NAME alpaca_markets_parse_benchmark COMMAND alpaca_markets_parse_benchmark $<$<CONFIG:Debug>:--no-timing>)
endif()
set_tests_properties(alpaca_markets_parse_benchmark PROPERTIES TIMEOUT 30)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(alpaca_markets_rest_probe rest_transport_probe.cpp)
target_link_libraries(alpaca_markets_rest_probe PRIVATE alpaca::markets)
add_test(NAME alpaca_markets_rest_transport
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_rest_transport.py"
        $<TARGET_FILE:alpaca_markets_rest_probe>)
set_tests_properties(alpaca_markets_rest_transport PROPERTIES TIMEOUT 30)
