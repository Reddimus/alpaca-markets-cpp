#include <alpaca/markets/markets.hpp>

#include <iostream>

int main() {
    auto env = alpaca::markets::Environment();
    if (auto status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::markets::Client(env);

    // Get the market clock
    auto clock_response = client.getClock();
    if (auto status = clock_response.first; !status.ok()) {
        std::cerr << "Error getting market clock: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto clock = clock_response.second;

    // Display market status
    std::cout << "Market is currently " << (clock.is_open ? "OPEN" : "CLOSED") << std::endl;
    std::cout << "Current timestamp: " << clock.timestamp << std::endl;
    std::cout << "Next open: " << clock.next_open << std::endl;
    std::cout << "Next close: " << clock.next_close << std::endl;

    return 0;
}
