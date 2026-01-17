#include <gtest/gtest.h>

#include <alpaca/markets/auction.hpp>

using namespace alpaca::markets;

TEST(AuctionTest, FromJSON) {
    Auction auction;
    std::string json = R"({
        "t": "2024-01-15T14:30:00Z",
        "p": 150.25,
        "s": 1000,
        "x": "N",
        "c": "O"
    })";

    ASSERT_TRUE(auction.fromJSON(json).ok());
    EXPECT_EQ(auction.timestamp, "2024-01-15T14:30:00Z");
    EXPECT_DOUBLE_EQ(auction.price, 150.25);
    EXPECT_EQ(auction.size, 1000);
    EXPECT_EQ(auction.exchange, "N");
    EXPECT_EQ(auction.condition, "O");
}

TEST(AuctionTest, FromJSONParseError) {
    Auction auction;
    std::string json = "invalid json";

    EXPECT_FALSE(auction.fromJSON(json).ok());
}

TEST(SymbolAuctionsTest, FromJSON) {
    SymbolAuctions symbol_auctions;
    std::string json = R"({
        "d": [
            {"t": "2024-01-15T09:30:00Z", "p": 150.00, "s": 500, "x": "N", "c": "O"},
            {"t": "2024-01-15T16:00:00Z", "p": 151.00, "s": 750, "x": "N", "c": "C"}
        ]
    })";

    ASSERT_TRUE(symbol_auctions.fromJSON(json).ok());
    EXPECT_EQ(symbol_auctions.daily_auctions.size(), 2);
    EXPECT_DOUBLE_EQ(symbol_auctions.daily_auctions[0].price, 150.00);
    EXPECT_DOUBLE_EQ(symbol_auctions.daily_auctions[1].price, 151.00);
}

TEST(AuctionsTest, FromJSON) {
    Auctions auctions;
    std::string json = R"({
        "auctions": {
            "AAPL": {
                "d": [
                    {"t": "2024-01-15T09:30:00Z", "p": 185.00, "s": 1000, "x": "N", "c": "O"}
                ]
            },
            "MSFT": {
                "d": [
                    {"t": "2024-01-15T09:30:00Z", "p": 375.00, "s": 800, "x": "N", "c": "O"}
                ]
            }
        },
        "next_page_token": "token123"
    })";

    ASSERT_TRUE(auctions.fromJSON(json).ok());
    EXPECT_EQ(auctions.auctions.size(), 2);
    EXPECT_TRUE(auctions.auctions.count("AAPL") > 0);
    EXPECT_TRUE(auctions.auctions.count("MSFT") > 0);
    EXPECT_DOUBLE_EQ(auctions.auctions["AAPL"].daily_auctions[0].price, 185.00);
    EXPECT_DOUBLE_EQ(auctions.auctions["MSFT"].daily_auctions[0].price, 375.00);
    EXPECT_EQ(auctions.next_page_token, "token123");
}
