# Examples CMakeLists.txt

# View account info example
add_executable(view_account_info view_account_info.cpp)
target_link_libraries(view_account_info PRIVATE alpaca_markets)

# Check market status example
add_executable(check_market_status check_market_status.cpp)
target_link_libraries(check_market_status PRIVATE alpaca_markets)

# Get latest quote example
add_executable(get_latest_quote get_latest_quote.cpp)
target_link_libraries(get_latest_quote PRIVATE alpaca_markets)
