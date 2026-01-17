#include <alpaca/markets/crypto.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(CryptoTradeTest, FromJSON) {
    const std::string json = R"({
        "t": "2024-01-10T15:30:00Z",
        "p": 42500.50,
        "s": 1500000,
        "i": 12345,
        "tks": "B"
    })";

    CryptoTrade trade;
    Status status = trade.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(trade.timestamp, "2024-01-10T15:30:00Z");
    EXPECT_DOUBLE_EQ(trade.price, 42500.50);
    EXPECT_EQ(trade.size, 1500000u);
    EXPECT_EQ(trade.id, 12345u);
    EXPECT_EQ(trade.taker_side, "B");
}

TEST(CryptoQuoteTest, FromJSON) {
    const std::string json = R"({
        "t": "2024-01-10T15:30:00Z",
        "ap": 42505.00,
        "as": 1.5,
        "bp": 42495.00,
        "bs": 2.3
    })";

    CryptoQuote quote;
    Status status = quote.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(quote.timestamp, "2024-01-10T15:30:00Z");
    EXPECT_DOUBLE_EQ(quote.ask_price, 42505.00);
    EXPECT_DOUBLE_EQ(quote.ask_size, 1.5);
    EXPECT_DOUBLE_EQ(quote.bid_price, 42495.00);
    EXPECT_DOUBLE_EQ(quote.bid_size, 2.3);
}

TEST(CryptoBarTest, FromJSON) {
    const std::string json = R"({
        "t": "2024-01-10T00:00:00Z",
        "o": 42000.00,
        "h": 43500.00,
        "l": 41500.00,
        "c": 43000.00,
        "v": 1250.75,
        "n": 50000,
        "vw": 42750.25
    })";

    CryptoBar bar;
    Status status = bar.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(bar.timestamp, "2024-01-10T00:00:00Z");
    EXPECT_DOUBLE_EQ(bar.open_price, 42000.00);
    EXPECT_DOUBLE_EQ(bar.high_price, 43500.00);
    EXPECT_DOUBLE_EQ(bar.low_price, 41500.00);
    EXPECT_DOUBLE_EQ(bar.close_price, 43000.00);
    EXPECT_DOUBLE_EQ(bar.volume, 1250.75);
    EXPECT_EQ(bar.trade_count, 50000u);
    EXPECT_DOUBLE_EQ(bar.vwap, 42750.25);
}

TEST(CryptoSnapshotTest, FromJSON) {
    const std::string json = R"({
        "latestTrade": {
            "t": "2024-01-10T15:30:00Z",
            "p": 42500.50,
            "s": 1500000,
            "tks": "B"
        },
        "latestQuote": {
            "t": "2024-01-10T15:30:01Z",
            "ap": 42505.00,
            "as": 1.5,
            "bp": 42495.00,
            "bs": 2.3
        },
        "minuteBar": {
            "t": "2024-01-10T15:30:00Z",
            "o": 42450.00,
            "h": 42510.00,
            "l": 42440.00,
            "c": 42500.00,
            "v": 15.5,
            "n": 500
        },
        "dailyBar": {
            "t": "2024-01-10T00:00:00Z",
            "o": 42000.00,
            "h": 43500.00,
            "l": 41500.00,
            "c": 42500.00,
            "v": 1250.75,
            "n": 50000
        }
    })";

    CryptoSnapshot snapshot;
    Status status = snapshot.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_DOUBLE_EQ(snapshot.latest_trade.price, 42500.50);
    EXPECT_EQ(snapshot.latest_trade.taker_side, "B");
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.ask_price, 42505.00);
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.close_price, 42500.00);
    EXPECT_DOUBLE_EQ(snapshot.daily_bar.volume, 1250.75);
}

TEST(CryptoTradesTest, FromJSON) {
    const std::string json = R"({
        "trades": {
            "BTC/USD": [
                {"t": "2024-01-10T15:30:00Z", "p": 42500.50, "s": 100, "tks": "B"},
                {"t": "2024-01-10T15:30:01Z", "p": 42510.00, "s": 50, "tks": "S"}
            ],
            "ETH/USD": [
                {"t": "2024-01-10T15:30:00Z", "p": 2250.00, "s": 1000, "tks": "B"}
            ]
        },
        "next_page_token": "token123"
    })";

    CryptoTrades trades;
    Status status = trades.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(trades.trades.size(), 2u);
    EXPECT_EQ(trades.trades["BTC/USD"].size(), 2u);
    EXPECT_EQ(trades.trades["ETH/USD"].size(), 1u);
    EXPECT_DOUBLE_EQ(trades.trades["BTC/USD"][0].price, 42500.50);
    EXPECT_EQ(trades.next_page_token, "token123");
}

TEST(CryptoBarsTest, FromJSON) {
    const std::string json = R"({
        "bars": {
            "BTC/USD": [
                {"t": "2024-01-10T00:00:00Z", "o": 42000, "h": 43000, "l": 41000, "c": 42500, "v": 100.5}
            ]
        },
        "next_page_token": "bartoken"
    })";

    CryptoBars bars;
    Status status = bars.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(bars.bars.size(), 1u);
    EXPECT_EQ(bars.bars["BTC/USD"].size(), 1u);
    EXPECT_DOUBLE_EQ(bars.bars["BTC/USD"][0].open_price, 42000.0);
    EXPECT_EQ(bars.next_page_token, "bartoken");
}

TEST(CryptoFeedTest, Conversions) {
    EXPECT_EQ(cryptoFeedToString(CryptoFeed::US), "us");
    EXPECT_EQ(cryptoFeedToString(CryptoFeed::Global), "global");
    
    EXPECT_EQ(stringToCryptoFeed("us"), CryptoFeed::US);
    EXPECT_EQ(stringToCryptoFeed("global"), CryptoFeed::Global);
    EXPECT_EQ(stringToCryptoFeed("unknown"), CryptoFeed::US);  // default
}

TEST(CryptoTradeTest, FromJSONParseError) {
    CryptoTrade trade;
    Status status = trade.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
