#include <alpaca/markets/quote.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(QuoteTest, FromJSON) {
    // Market Data API v2 format
    const std::string json = R"({
        "ap": 150.55,
        "as": 200,
        "ax": "Q",
        "bp": 150.50,
        "bs": 300,
        "bx": "P",
        "t": "2023-01-01T10:00:00Z",
        "c": ["R"]
    })";

    Quote quote;
    Status status = quote.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_DOUBLE_EQ(quote.ask_price, 150.55);
    EXPECT_EQ(quote.ask_size, 200u);
    EXPECT_EQ(quote.ask_exchange, "Q");
    EXPECT_DOUBLE_EQ(quote.bid_price, 150.50);
    EXPECT_EQ(quote.bid_size, 300u);
    EXPECT_EQ(quote.bid_exchange, "P");
    EXPECT_EQ(quote.timestamp, "2023-01-01T10:00:00Z");
    EXPECT_EQ(quote.conditions.size(), 1u);
    EXPECT_EQ(quote.conditions[0], "R");
}

TEST(LatestQuoteTest, FromJSON) {
    // Market Data API v2 latest quote response
    const std::string json = R"({
        "symbol": "AAPL",
        "quote": {
            "ap": 150.55,
            "as": 200,
            "ax": "Q",
            "bp": 150.50,
            "bs": 300,
            "bx": "P",
            "t": "2023-01-01T10:00:00Z",
            "c": []
        }
    })";

    LatestQuote latest_quote;
    Status status = latest_quote.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(latest_quote.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(latest_quote.quote.ask_price, 150.55);
    EXPECT_DOUBLE_EQ(latest_quote.quote.bid_price, 150.50);
}

TEST(QuoteTest, FromJSONParseError) {
    Quote quote;
    Status status = quote.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
