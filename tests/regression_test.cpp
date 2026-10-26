#include <alpaca/markets/bars.hpp>
#include <alpaca/markets/crypto.hpp>
#include <alpaca/markets/quote.hpp>
#include <alpaca/markets/trade.hpp>

#include <gtest/gtest.h>

#include <limits>

TEST(JsonRegression, PreservesFullWidthTradeIds) {
    alpaca::markets::Trade trade;
    ASSERT_TRUE(trade.fromJSON(R"({"i":18446744073709551615,"s":1,"p":2.5})").ok());
    EXPECT_EQ(trade.id, std::numeric_limits<std::uint64_t>::max());
}
TEST(JsonRegression, RejectsFractionalAndOutOfRangeIntegers) {
    alpaca::markets::Trade trade;
    EXPECT_FALSE(trade.fromJSON(R"({"i":9.5})").ok());
    EXPECT_FALSE(trade.fromJSON(R"({"i":-1})").ok());
    EXPECT_FALSE(trade.fromJSON(R"({"i":1e300})").ok());
}
TEST(JsonRegression, ReusedModelsAndTrailingDataAreHandled) {
    alpaca::markets::Trade trade;
    ASSERT_TRUE(trade.fromJSON(R"({"i":123,"p":4.5})").ok());
    ASSERT_TRUE(trade.fromJSON("{}").ok());
    EXPECT_EQ(trade.id, 0);
    EXPECT_EQ(trade.price, 0);
    EXPECT_FALSE(trade.fromJSON("{} garbage").ok());
}

TEST(JsonRegression, PreservesFractionalContractQuantities) {
    alpaca::markets::Trade stock;
    alpaca::markets::CryptoTrade crypto;
    alpaca::markets::Quote quote;
    alpaca::markets::Bar bar;
    ASSERT_TRUE(stock.fromJSON(R"({"s":0.25})").ok());
    ASSERT_TRUE(crypto.fromJSON(R"({"s":0.0003})").ok());
    ASSERT_TRUE(quote.fromJSON(R"({"as":0.25,"bs":0.5})").ok());
    ASSERT_TRUE(bar.fromJSON(R"({"v":0.75})").ok());
    EXPECT_DOUBLE_EQ(stock.size, 0.25);
    EXPECT_DOUBLE_EQ(crypto.size, 0.0003);
    EXPECT_DOUBLE_EQ(quote.ask_size, 0.25);
    EXPECT_DOUBLE_EQ(quote.bid_size, 0.5);
    EXPECT_DOUBLE_EQ(bar.volume, 0.75);
}
