#include <alpaca/markets/markets.hpp>

#include <iostream>

int main() {
    alpaca::markets::Environment env;
    if (alpaca::markets::Status status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::Client client(env);

    // Get our account information.
    std::pair<alpaca::markets::Status, alpaca::markets::Account> account_response = client.getAccount();
    if (alpaca::markets::Status status = account_response.first; !status.ok()) {
        std::cerr << "Error getting account information: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::Account account = account_response.second;

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
