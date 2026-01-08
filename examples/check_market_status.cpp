#include <alpaca/markets/markets.hpp>

#include <iostream>

int main() {
    alpaca::markets::Environment env;
    if (alpaca::markets::Status status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::Client client(env);

    // Get the market clock
    std::pair<alpaca::markets::Status, alpaca::markets::Clock> clock_response = client.getClock();
    if (alpaca::markets::Status status = clock_response.first; !status.ok()) {
        std::cerr << "Error getting market clock: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::Clock clock = clock_response.second;

    // Display market status
    std::cout << "Market is currently " << (clock.is_open ? "OPEN" : "CLOSED") << std::endl;
    std::cout << "Current timestamp: " << clock.timestamp << std::endl;
    std::cout << "Next open: " << clock.next_open << std::endl;
    std::cout << "Next close: " << clock.next_close << std::endl;

    return 0;
}
