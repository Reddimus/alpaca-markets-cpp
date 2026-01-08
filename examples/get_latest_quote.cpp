#include <alpaca/markets/markets.hpp>

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <symbol>" << std::endl;
        std::cerr << "Example: " << argv[0] << " AAPL" << std::endl;
        return 1;
    }

    std::string symbol = argv[1];

    alpaca::markets::Environment env;
    if (alpaca::markets::Status status = env.parse(); !status.ok()) {
        std::cerr << "Error parsing config from environment: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::Client client(env);

    // Get latest quote using Market Data API v2
    std::pair<alpaca::markets::Status, alpaca::markets::LatestQuote> quote_response = client.getLatestQuote(symbol);
    if (alpaca::markets::Status status = quote_response.first; !status.ok()) {
        std::cerr << "Error getting latest quote: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::LatestQuote latest_quote = quote_response.second;

    // Display quote information
    std::cout << "Latest quote for " << symbol << ":" << std::endl;
    std::cout << "  Bid: $" << latest_quote.quote.bid_price << " x " << latest_quote.quote.bid_size << std::endl;
    std::cout << "  Ask: $" << latest_quote.quote.ask_price << " x " << latest_quote.quote.ask_size << std::endl;
    std::cout << "  Timestamp: " << latest_quote.quote.timestamp << std::endl;

    // Also get latest trade
    std::pair<alpaca::markets::Status, alpaca::markets::LatestTrade> trade_response = client.getLatestTrade(symbol);
    if (alpaca::markets::Status status = trade_response.first; !status.ok()) {
        std::cerr << "Error getting latest trade: " << status.getMessage() << std::endl;
        return status.getCode();
    }
    alpaca::markets::LatestTrade latest_trade = trade_response.second;

    std::cout << "Latest trade for " << symbol << ":" << std::endl;
    std::cout << "  Price: $" << latest_trade.trade.price << std::endl;
    std::cout << "  Size: " << latest_trade.trade.size << std::endl;
    std::cout << "  Timestamp: " << latest_trade.trade.timestamp << std::endl;

    return 0;
}
