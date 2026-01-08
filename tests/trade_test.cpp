#include <alpaca/markets/trade.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(TradeTest, FromJSON) {
    // Market Data API v2 format
    const std::string json = R"({
        "p": 150.50,
        "s": 100,
        "x": "V",
        "i": 123456789,
        "t": "2023-01-01T10:00:00Z",
        "c": ["@", "F"],
        "z": "A"
    })";

    Trade trade;
    Status status = trade.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_DOUBLE_EQ(trade.price, 150.50);
    EXPECT_EQ(trade.size, 100u);
    EXPECT_EQ(trade.exchange, "V");
    EXPECT_EQ(trade.id, 123456789u);
    EXPECT_EQ(trade.timestamp, "2023-01-01T10:00:00Z");
    EXPECT_EQ(trade.conditions.size(), 2u);
    EXPECT_EQ(trade.conditions[0], "@");
    EXPECT_EQ(trade.conditions[1], "F");
    EXPECT_EQ(trade.tape, "A");
}

TEST(LatestTradeTest, FromJSON) {
    // Market Data API v2 latest trade response
    const std::string json = R"({
        "symbol": "AAPL",
        "trade": {
            "p": 150.50,
            "s": 100,
            "x": "V",
            "i": 123456789,
            "t": "2023-01-01T10:00:00Z",
            "c": ["@"],
            "z": "A"
        }
    })";

    LatestTrade latest_trade;
    Status status = latest_trade.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(latest_trade.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(latest_trade.trade.price, 150.50);
    EXPECT_EQ(latest_trade.trade.size, 100u);
}

TEST(TradeTest, FromJSONParseError) {
    Trade trade;
    Status status = trade.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
