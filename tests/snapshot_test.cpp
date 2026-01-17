#include <alpaca/markets/snapshot.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(SnapshotTest, FromJSON) {
    const std::string json = R"({
        "latestTrade": {
            "t": "2024-01-10T15:30:00Z",
            "p": 185.50,
            "s": 100,
            "x": "V",
            "i": 12345,
            "c": ["@"],
            "z": "A"
        },
        "latestQuote": {
            "t": "2024-01-10T15:30:01Z",
            "ap": 185.55,
            "as": 500,
            "ax": "Q",
            "bp": 185.45,
            "bs": 300,
            "bx": "P",
            "c": ["R"]
        },
        "minuteBar": {
            "t": "2024-01-10T15:30:00Z",
            "o": 185.00,
            "h": 185.75,
            "l": 184.90,
            "c": 185.50,
            "v": 50000,
            "n": 250,
            "vw": 185.25
        },
        "dailyBar": {
            "t": "2024-01-10T00:00:00Z",
            "o": 183.00,
            "h": 186.00,
            "l": 182.50,
            "c": 185.50,
            "v": 5000000,
            "n": 25000,
            "vw": 184.75
        },
        "prevDailyBar": {
            "t": "2024-01-09T00:00:00Z",
            "o": 181.00,
            "h": 184.00,
            "l": 180.00,
            "c": 183.00,
            "v": 4500000,
            "n": 22000,
            "vw": 182.50
        }
    })";

    Snapshot snapshot;
    Status status = snapshot.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    
    // Verify latest trade
    EXPECT_EQ(snapshot.latest_trade.timestamp, "2024-01-10T15:30:00Z");
    EXPECT_DOUBLE_EQ(snapshot.latest_trade.price, 185.50);
    EXPECT_EQ(snapshot.latest_trade.size, 100u);
    EXPECT_EQ(snapshot.latest_trade.exchange, "V");
    
    // Verify latest quote
    EXPECT_EQ(snapshot.latest_quote.timestamp, "2024-01-10T15:30:01Z");
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.ask_price, 185.55);
    EXPECT_EQ(snapshot.latest_quote.ask_size, 500u);
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.bid_price, 185.45);
    EXPECT_EQ(snapshot.latest_quote.bid_size, 300u);
    
    // Verify minute bar
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.open_price, 185.00);
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.high_price, 185.75);
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.close_price, 185.50);
    EXPECT_EQ(snapshot.minute_bar.volume, 50000u);
    
    // Verify daily bar
    EXPECT_DOUBLE_EQ(snapshot.daily_bar.open_price, 183.00);
    EXPECT_DOUBLE_EQ(snapshot.daily_bar.close_price, 185.50);
    EXPECT_EQ(snapshot.daily_bar.volume, 5000000u);
    
    // Verify previous daily bar
    EXPECT_DOUBLE_EQ(snapshot.prev_daily_bar.open_price, 181.00);
    EXPECT_DOUBLE_EQ(snapshot.prev_daily_bar.close_price, 183.00);
}

TEST(SnapshotsTest, FromJSON) {
    const std::string json = R"({
        "snapshots": {
            "AAPL": {
                "latestTrade": {
                    "t": "2024-01-10T15:30:00Z",
                    "p": 185.50,
                    "s": 100,
                    "x": "V"
                },
                "latestQuote": {
                    "t": "2024-01-10T15:30:01Z",
                    "ap": 185.55,
                    "as": 500,
                    "bp": 185.45,
                    "bs": 300
                },
                "minuteBar": {
                    "t": "2024-01-10T15:30:00Z",
                    "o": 185.00,
                    "h": 185.75,
                    "l": 184.90,
                    "c": 185.50,
                    "v": 50000
                }
            },
            "GOOG": {
                "latestTrade": {
                    "t": "2024-01-10T15:30:00Z",
                    "p": 142.25,
                    "s": 50,
                    "x": "Q"
                },
                "latestQuote": {
                    "t": "2024-01-10T15:30:01Z",
                    "ap": 142.30,
                    "as": 200,
                    "bp": 142.20,
                    "bs": 150
                },
                "minuteBar": {
                    "t": "2024-01-10T15:30:00Z",
                    "o": 141.50,
                    "h": 142.50,
                    "l": 141.25,
                    "c": 142.25,
                    "v": 30000
                }
            }
        }
    })";

    Snapshots snapshots;
    Status status = snapshots.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(snapshots.snapshots.size(), 2u);
    
    // Verify AAPL snapshot
    EXPECT_TRUE(snapshots.snapshots.count("AAPL") > 0);
    EXPECT_DOUBLE_EQ(snapshots.snapshots["AAPL"].latest_trade.price, 185.50);
    EXPECT_DOUBLE_EQ(snapshots.snapshots["AAPL"].latest_quote.ask_price, 185.55);
    
    // Verify GOOG snapshot
    EXPECT_TRUE(snapshots.snapshots.count("GOOG") > 0);
    EXPECT_DOUBLE_EQ(snapshots.snapshots["GOOG"].latest_trade.price, 142.25);
    EXPECT_DOUBLE_EQ(snapshots.snapshots["GOOG"].latest_quote.bid_price, 142.20);
}

TEST(SnapshotTest, PartialData) {
    // Test snapshot with only some fields present
    const std::string json = R"({
        "latestTrade": {
            "t": "2024-01-10T15:30:00Z",
            "p": 185.50,
            "s": 100,
            "x": "V"
        }
    })";

    Snapshot snapshot;
    Status status = snapshot.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_DOUBLE_EQ(snapshot.latest_trade.price, 185.50);
    // Other fields should be default values
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.ask_price, 0.0);
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.open_price, 0.0);
}

TEST(SnapshotTest, FromJSONParseError) {
    Snapshot snapshot;
    Status status = snapshot.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
