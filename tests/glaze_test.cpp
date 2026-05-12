// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Glaze-deserializer shape-parity tests. Verifies the post-migration
// fromJSON() output matches the pre-migration RapidJSON path on the
// fixtures most likely to drift: empty fields, missing optional keys,
// the Market Data API v2 nested {trade}/{quote}/{bars} envelopes, and
// the streaming envelope's {stream, data} discriminator.

#include <alpaca/markets/account.hpp>
#include <alpaca/markets/bars.hpp>
#include <alpaca/markets/news.hpp>
#include <alpaca/markets/option.hpp>
#include <alpaca/markets/order.hpp>
#include <alpaca/markets/quote.hpp>
#include <alpaca/markets/snapshot.hpp>
#include <alpaca/markets/streaming.hpp>
#include <alpaca/markets/trade.hpp>

#include <gtest/gtest.h>

#include <string>

namespace alpaca::markets {
namespace {

// ---- Bars: Market Data v2 multi-symbol envelope ----

TEST(GlazeMigrationTest, BarsByMultiSymbolPayload) {
    const std::string json = R"({
        "bars": {
            "AAPL": [
                {"t": "2026-01-01T09:30:00Z", "o": 150.25, "h": 152.0, "l": 149.5, "c": 151.75,
                 "v": 1000000, "n": 5000, "vw": 151.0},
                {"t": "2026-01-01T09:31:00Z", "o": 151.75, "h": 152.5, "l": 151.5, "c": 152.0,
                 "v": 800000, "n": 4000, "vw": 152.0}
            ],
            "GOOG": [
                {"t": "2026-01-01T09:30:00Z", "o": 2800.0, "h": 2850.0, "l": 2790.0, "c": 2840.0,
                 "v": 500000, "n": 2000, "vw": 2820.0}
            ]
        },
        "next_page_token": "tok-123"
    })";

    Bars bars;
    Status status = bars.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(bars.bars.size(), 2u);
    ASSERT_EQ(bars.bars["AAPL"].size(), 2u);
    EXPECT_DOUBLE_EQ(bars.bars["AAPL"][0].open_price, 150.25);
    EXPECT_DOUBLE_EQ(bars.bars["AAPL"][1].close_price, 152.0);
    EXPECT_EQ(bars.bars["AAPL"][0].volume, 1000000u);
    EXPECT_EQ(bars.bars["AAPL"][0].trade_count, 5000u);
    ASSERT_EQ(bars.bars["GOOG"].size(), 1u);
    EXPECT_DOUBLE_EQ(bars.bars["GOOG"][0].high_price, 2850.0);
    EXPECT_EQ(bars.next_page_token, "tok-123");
}

TEST(GlazeMigrationTest, BarsRejectsInvalidJson) {
    Bars bars;
    EXPECT_FALSE(bars.fromJSON("not json").ok());
    EXPECT_FALSE(bars.fromJSON("[]").ok());  // Wrong type at top level
}

// ---- Account: scalar/bool field mix ----

TEST(GlazeMigrationTest, AccountFullPayload) {
    const std::string json = R"({
        "id": "abc-123",
        "account_number": "123456789",
        "status": "ACTIVE",
        "currency": "USD",
        "cash": "50000.00",
        "buying_power": "100000.00",
        "equity": "100000.00",
        "daytrade_count": 0,
        "pattern_day_trader": false,
        "shorting_enabled": true,
        "account_blocked": false,
        "trading_blocked": false,
        "transfers_blocked": false
    })";

    Account account;
    Status status = account.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(account.id, "abc-123");
    EXPECT_EQ(account.account_number, "123456789");
    EXPECT_EQ(account.status, "ACTIVE");
    EXPECT_EQ(account.cash, "50000.00");
    EXPECT_EQ(account.daytrade_count, 0);
    EXPECT_FALSE(account.pattern_day_trader);
    EXPECT_TRUE(account.shorting_enabled);
}

// ---- LatestQuote: nested "quote" sub-object envelope ----

TEST(GlazeMigrationTest, LatestQuoteNestedEnvelope) {
    const std::string json = R"({
        "symbol": "AAPL",
        "quote": {
            "ap": 152.5, "as": 100, "ax": "Q",
            "bp": 152.4, "bs": 50, "bx": "N",
            "t": "2026-01-01T09:30:00Z",
            "c": ["R", "F"]
        }
    })";

    LatestQuote lq;
    Status status = lq.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(lq.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(lq.quote.ask_price, 152.5);
    EXPECT_EQ(lq.quote.ask_size, 100u);
    EXPECT_EQ(lq.quote.ask_exchange, "Q");
    EXPECT_DOUBLE_EQ(lq.quote.bid_price, 152.4);
    ASSERT_EQ(lq.quote.conditions.size(), 2u);
    EXPECT_EQ(lq.quote.conditions[0], "R");
}

TEST(GlazeMigrationTest, LatestTradeNestedEnvelope) {
    const std::string json = R"({
        "symbol": "AAPL",
        "trade": {
            "p": 151.75, "s": 100, "x": "Q", "i": 99999,
            "t": "2026-01-01T09:30:00Z", "c": ["@"], "z": "C"
        }
    })";

    LatestTrade lt;
    Status status = lt.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(lt.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(lt.trade.price, 151.75);
    EXPECT_EQ(lt.trade.size, 100u);
    EXPECT_EQ(lt.trade.id, 99999u);
    EXPECT_EQ(lt.trade.tape, "C");
}

// ---- Snapshot: 5 nested sub-models ----

TEST(GlazeMigrationTest, SnapshotAllSubFields) {
    const std::string json = R"({
        "latestTrade": {"p": 151.75, "s": 100, "x": "Q", "i": 1, "t": "2026-01-01T09:30:00Z",
                        "c": ["@"], "z": "C"},
        "latestQuote": {"ap": 152.5, "as": 100, "bp": 152.4, "bs": 50,
                        "t": "2026-01-01T09:30:00Z"},
        "minuteBar":   {"t": "2026-01-01T09:30:00Z", "o": 150.0, "h": 152.0, "l": 149.5,
                        "c": 151.75, "v": 100000, "n": 1000, "vw": 151.0},
        "dailyBar":    {"t": "2026-01-01T00:00:00Z", "o": 145.0, "h": 152.0, "l": 144.0,
                        "c": 151.75, "v": 10000000, "n": 50000, "vw": 148.0},
        "prevDailyBar":{"t": "2025-12-31T00:00:00Z", "o": 140.0, "h": 146.0, "l": 139.0,
                        "c": 145.0,  "v": 9000000, "n": 45000, "vw": 142.0}
    })";

    Snapshot snapshot;
    Status status = snapshot.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_DOUBLE_EQ(snapshot.latest_trade.price, 151.75);
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.ask_price, 152.5);
    EXPECT_DOUBLE_EQ(snapshot.minute_bar.close_price, 151.75);
    EXPECT_DOUBLE_EQ(snapshot.daily_bar.high_price, 152.0);
    EXPECT_DOUBLE_EQ(snapshot.prev_daily_bar.close_price, 145.0);
}

TEST(GlazeMigrationTest, SnapshotMissingSubFieldsOK) {
    // Only latestTrade present, rest should default-init.
    const std::string json = R"({
        "latestTrade": {"p": 99.99, "s": 1, "x": "Q", "i": 1, "t": "2026-01-01T09:30:00Z",
                        "c": ["@"], "z": "C"}
    })";

    Snapshot snapshot;
    Status status = snapshot.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_DOUBLE_EQ(snapshot.latest_trade.price, 99.99);
    EXPECT_DOUBLE_EQ(snapshot.latest_quote.ask_price, 0.0);
    EXPECT_DOUBLE_EQ(snapshot.daily_bar.open_price, 0.0);
}

// ---- Order: dense string-typed field mix ----

TEST(GlazeMigrationTest, OrderTypicalMarketOrder) {
    const std::string json = R"({
        "id": "ord-1",
        "client_order_id": "cli-1",
        "symbol": "AAPL",
        "qty": "10",
        "filled_qty": "10",
        "filled_avg_price": "151.75",
        "side": "buy",
        "type": "market",
        "time_in_force": "day",
        "status": "filled",
        "extended_hours": false,
        "asset_class": "us_equity"
    })";

    Order order;
    Status status = order.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(order.id, "ord-1");
    EXPECT_EQ(order.client_order_id, "cli-1");
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.filled_avg_price, "151.75");
    EXPECT_EQ(order.status, "filled");
    EXPECT_FALSE(order.extended_hours);
}

// ---- News: array-of-images sub-records ----

TEST(GlazeMigrationTest, NewsWithImagesAndSymbols) {
    const std::string json = R"({
        "id": 12345,
        "headline": "Apple Reports Earnings",
        "author": "AP",
        "summary": "AAPL beat estimates.",
        "url": "https://example.com",
        "source": "alpaca",
        "symbols": ["AAPL", "QQQ"],
        "images": [
            {"size": "large", "url": "https://example.com/large.jpg"},
            {"size": "thumb", "url": "https://example.com/thumb.jpg"}
        ]
    })";

    News news;
    Status status = news.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(news.id, 12345u);
    EXPECT_EQ(news.headline, "Apple Reports Earnings");
    ASSERT_EQ(news.symbols.size(), 2u);
    EXPECT_EQ(news.symbols[0], "AAPL");
    ASSERT_EQ(news.images.size(), 2u);
    EXPECT_EQ(news.images[0].size, "large");
    EXPECT_EQ(news.images[1].url, "https://example.com/thumb.jpg");
}

// ---- OptionContract: string-typed enum fields + nested deliverables array ----

TEST(GlazeMigrationTest, OptionContractWithDeliverables) {
    const std::string json = R"({
        "id": "opt-1",
        "symbol": "AAPL250117C00150000",
        "name": "AAPL Jan 25 150 Call",
        "tradable": true,
        "type": "call",
        "style": "american",
        "status": "active",
        "strike_price": "150.00",
        "expiration_date": "2025-01-17",
        "deliverables": [
            {"type": "equity", "symbol": "AAPL", "amount": "100",
             "delayed_settlement": false}
        ]
    })";

    OptionContract contract;
    Status status = contract.fromJSON(json);
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(contract.id, "opt-1");
    EXPECT_EQ(contract.symbol, "AAPL250117C00150000");
    EXPECT_TRUE(contract.tradable);
    EXPECT_EQ(contract.type, OptionType::Call);
    EXPECT_EQ(contract.style, OptionStyle::American);
    EXPECT_EQ(contract.status, OptionStatus::Active);
    ASSERT_EQ(contract.deliverables.size(), 1u);
    EXPECT_EQ(contract.deliverables[0].type, "equity");
    EXPECT_EQ(contract.deliverables[0].amount, "100");
}

// ---- Streaming reply envelope: typed-tag discriminator ----
//
// The reply envelope has the shape {"stream": "<name>", "data": <obj>}.
// This is exactly the case where Glaze's tagged-union support would
// shine, *if* we had typed alternatives for "data" declared in this
// SDK. We don't (downstream consumers parse the data sub-object with
// their own schemas), so we use glz::generic for the outer envelope
// and pass the data sub-object through as a JSON string. Verify the
// data passthrough preserves the original JSON content.

TEST(GlazeMigrationTest, StreamReplyAuthorization) {
    using namespace stream;
    auto [status, reply] = parseReply(R"({"stream": "authorization", "data": {}})");
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(reply.reply_type, ReplyType::Authorization);
}

TEST(GlazeMigrationTest, StreamReplyTradeUpdateDataPassthrough) {
    using namespace stream;
    auto [status, reply] = parseReply(
        R"({"stream": "trade_updates", "data": {"event": "fill", "order": {"id": "ord-9"}}})");
    ASSERT_TRUE(status.ok()) << status.getMessage();
    EXPECT_EQ(reply.reply_type, ReplyType::Update);
    EXPECT_EQ(reply.stream_type, StreamType::TradeUpdates);
    // The data passthrough should preserve "fill" - exact JSON formatting
    // differs between RapidJSON's compact-writer and Glaze's, so we only
    // assert presence rather than string equality.
    EXPECT_NE(reply.data.find("\"fill\""), std::string::npos);
    EXPECT_NE(reply.data.find("ord-9"), std::string::npos);
}

TEST(GlazeMigrationTest, StreamReplyRejectsUnknownStream) {
    using namespace stream;
    auto [status, reply] = parseReply(R"({"stream": "unknown_thing", "data": {}})");
    EXPECT_FALSE(status.ok());
}

}  // namespace
}  // namespace alpaca::markets
