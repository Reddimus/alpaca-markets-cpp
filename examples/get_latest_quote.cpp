#include <alpaca/markets/markets.hpp>

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <symbol>" << std::endl;
        std::cerr << "Example: " << argv[0] << " AAPL" << std::endl;
        return 1;
    }

    std::string symbol = argv[1];

    auto env = alpaca::markets::Environment();
    if (auto status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto client = alpaca::markets::Client(env);

    // Get latest quote using Market Data API v2
    auto quote_response = client.getLatestQuote(symbol);
    if (auto status = quote_response.first; !status.ok()) {
        std::cerr << "Error getting latest quote: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto latest_quote = quote_response.second;

    // Display quote information
    std::cout << "Latest quote for " << symbol << ":" << std::endl;
    std::cout << "  Bid: $" << latest_quote.quote.bid_price << " x " << latest_quote.quote.bid_size << std::endl;
    std::cout << "  Ask: $" << latest_quote.quote.ask_price << " x " << latest_quote.quote.ask_size << std::endl;
    std::cout << "  Timestamp: " << latest_quote.quote.timestamp << std::endl;

    // Also get latest trade
    auto trade_response = client.getLatestTrade(symbol);
    if (auto status = trade_response.first; !status.ok()) {
        std::cerr << "Error getting latest trade: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    auto latest_trade = trade_response.second;

    std::cout << "Latest trade for " << symbol << ":" << std::endl;
    std::cout << "  Price: $" << latest_trade.trade.price << std::endl;
    std::cout << "  Size: " << latest_trade.trade.size << std::endl;
    std::cout << "  Timestamp: " << latest_trade.trade.timestamp << std::endl;

    return 0;
}
