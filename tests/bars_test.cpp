#include <alpaca/markets/bars.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(BarTest, FromJSON) {
    // Market Data API v2 format
    const std::string json = R"({
        "t": "2023-01-01T09:30:00Z",
        "o": 150.25,
        "h": 152.00,
        "l": 149.50,
        "c": 151.75,
        "v": 1000000,
        "n": 5000,
        "vw": 151.00
    })";

    Bar bar;
    auto status = bar.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(bar.timestamp, "2023-01-01T09:30:00Z");
    EXPECT_DOUBLE_EQ(bar.open_price, 150.25);
    EXPECT_DOUBLE_EQ(bar.high_price, 152.00);
    EXPECT_DOUBLE_EQ(bar.low_price, 149.50);
    EXPECT_DOUBLE_EQ(bar.close_price, 151.75);
    EXPECT_EQ(bar.volume, 1000000u);
    EXPECT_EQ(bar.trade_count, 5000u);
    EXPECT_DOUBLE_EQ(bar.vwap, 151.00);
}

TEST(BarsTest, FromJSON) {
    // Market Data API v2 multi-symbol response
    const std::string json = R"({
        "bars": {
            "AAPL": [
                {
                    "t": "2023-01-01T09:30:00Z",
                    "o": 150.25,
                    "h": 152.00,
                    "l": 149.50,
                    "c": 151.75,
                    "v": 1000000,
                    "n": 5000,
                    "vw": 151.00
                }
            ],
            "GOOG": [
                {
                    "t": "2023-01-01T09:30:00Z",
                    "o": 2800.00,
                    "h": 2850.00,
                    "l": 2790.00,
                    "c": 2840.00,
                    "v": 500000,
                    "n": 2000,
                    "vw": 2820.00
                }
            ]
        },
        "next_page_token": "token123"
    })";

    Bars bars;
    auto status = bars.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(bars.bars.size(), 2u);
    EXPECT_EQ(bars.bars["AAPL"].size(), 1u);
    EXPECT_EQ(bars.bars["GOOG"].size(), 1u);
    EXPECT_EQ(bars.next_page_token, "token123");
    
    auto& aapl_bar = bars.bars["AAPL"][0];
    EXPECT_DOUBLE_EQ(aapl_bar.open_price, 150.25);
    EXPECT_DOUBLE_EQ(aapl_bar.close_price, 151.75);
}

TEST(BarTest, FromJSONParseError) {
    Bar bar;
    auto status = bar.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
