#include <alpaca/markets/markets.hpp>

#include <iostream>

int main() {
    auto env = alpaca::markets::Environment();
    if (auto status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::markets::Client(env);

    // Get our account information.
    auto account_response = client.getAccount();
    if (auto status = account_response.first; !status.ok()) {
        std::cerr << "Error getting account information: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto account = account_response.second;

    // Check if our account is restricted from trading.
    if (account.trading_blocked) {
        std::cout << "Account is currently restricted from trading." << std::endl;
    }

    // Display account information
    std::cout << "Account ID: " << account.id << std::endl;
    std::cout << "Account Status: " << account.status << std::endl;
    std::cout << "Buying Power: $" << account.buying_power << std::endl;
    std::cout << "Cash: $" << account.cash << std::endl;
    std::cout << "Equity: $" << account.equity << std::endl;
    std::cout << "Currency: " << account.currency << std::endl;

    return 0;
}
