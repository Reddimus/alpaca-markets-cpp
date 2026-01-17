#include <gtest/gtest.h>

#include <alpaca/markets/multi_trade.hpp>
#include <alpaca/markets/multi_quote.hpp>

using namespace alpaca::markets;

TEST(MultiTradesTest, FromJSON) {
    MultiTrades multi_trades;
    std::string json = R"({
        "trades": {
            "AAPL": [
                {"t": "2024-01-15T14:30:00Z", "p": 185.50, "s": 100, "x": "N", "i": 12345, "c": ["@"], "z": "A"},
                {"t": "2024-01-15T14:30:01Z", "p": 185.55, "s": 200, "x": "Q", "i": 12346, "c": ["@"], "z": "A"}
            ],
            "MSFT": [
                {"t": "2024-01-15T14:30:00Z", "p": 375.00, "s": 50, "x": "N", "i": 22345, "c": ["@"], "z": "A"}
            ]
        },
        "next_page_token": "token456"
    })";

    ASSERT_TRUE(multi_trades.fromJSON(json).ok());
    EXPECT_EQ(multi_trades.trades.size(), 2);
    EXPECT_TRUE(multi_trades.trades.count("AAPL") > 0);
    EXPECT_TRUE(multi_trades.trades.count("MSFT") > 0);
    EXPECT_EQ(multi_trades.trades["AAPL"].size(), 2);
    EXPECT_EQ(multi_trades.trades["MSFT"].size(), 1);
    EXPECT_DOUBLE_EQ(multi_trades.trades["AAPL"][0].price, 185.50);
    EXPECT_DOUBLE_EQ(multi_trades.trades["AAPL"][1].price, 185.55);
    EXPECT_DOUBLE_EQ(multi_trades.trades["MSFT"][0].price, 375.00);
    EXPECT_EQ(multi_trades.next_page_token, "token456");
}

TEST(MultiTradesTest, FromJSONParseError) {
    MultiTrades multi_trades;
    std::string json = "not valid json";

    EXPECT_FALSE(multi_trades.fromJSON(json).ok());
}

TEST(MultiTradesTest, EmptyTrades) {
    MultiTrades multi_trades;
    std::string json = R"({
        "trades": {}
    })";

    ASSERT_TRUE(multi_trades.fromJSON(json).ok());
    EXPECT_EQ(multi_trades.trades.size(), 0);
    EXPECT_TRUE(multi_trades.next_page_token.empty());
}

TEST(MultiQuotesTest, FromJSON) {
    MultiQuotes multi_quotes;
    std::string json = R"({
        "quotes": {
            "AAPL": [
                {"t": "2024-01-15T14:30:00Z", "ap": 185.55, "as": 100, "ax": "N", "bp": 185.50, "bs": 200, "bx": "Q", "c": ["R"]},
                {"t": "2024-01-15T14:30:01Z", "ap": 185.60, "as": 150, "ax": "N", "bp": 185.55, "bs": 250, "bx": "Q", "c": ["R"]}
            ],
            "MSFT": [
                {"t": "2024-01-15T14:30:00Z", "ap": 375.10, "as": 50, "ax": "N", "bp": 375.00, "bs": 100, "bx": "N", "c": ["R"]}
            ]
        },
        "next_page_token": "quoteToken789"
    })";

    ASSERT_TRUE(multi_quotes.fromJSON(json).ok());
    EXPECT_EQ(multi_quotes.quotes.size(), 2);
    EXPECT_TRUE(multi_quotes.quotes.count("AAPL") > 0);
    EXPECT_TRUE(multi_quotes.quotes.count("MSFT") > 0);
    EXPECT_EQ(multi_quotes.quotes["AAPL"].size(), 2);
    EXPECT_EQ(multi_quotes.quotes["MSFT"].size(), 1);
    EXPECT_DOUBLE_EQ(multi_quotes.quotes["AAPL"][0].ask_price, 185.55);
    EXPECT_DOUBLE_EQ(multi_quotes.quotes["AAPL"][0].bid_price, 185.50);
    EXPECT_DOUBLE_EQ(multi_quotes.quotes["MSFT"][0].ask_price, 375.10);
    EXPECT_EQ(multi_quotes.next_page_token, "quoteToken789");
}

TEST(MultiQuotesTest, FromJSONParseError) {
    MultiQuotes multi_quotes;
    std::string json = "not valid json";

    EXPECT_FALSE(multi_quotes.fromJSON(json).ok());
}

TEST(MultiQuotesTest, EmptyQuotes) {
    MultiQuotes multi_quotes;
    std::string json = R"({
        "quotes": {}
    })";

    ASSERT_TRUE(multi_quotes.fromJSON(json).ok());
    EXPECT_EQ(multi_quotes.quotes.size(), 0);
    EXPECT_TRUE(multi_quotes.next_page_token.empty());
}
