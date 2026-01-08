#include <alpaca/markets/order.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(OrderTest, FromJSON) {
    const std::string json = R"({
        "asset_class": "us_equity",
        "asset_id": "asset-123",
        "canceled_at": null,
        "client_order_id": "client-456",
        "created_at": "2020-01-01T10:00:00Z",
        "expired_at": null,
        "extended_hours": false,
        "failed_at": null,
        "filled_at": "2020-01-01T10:00:01Z",
        "filled_avg_price": "150.00",
        "filled_qty": "10",
        "id": "order-789",
        "legs": false,
        "limit_price": null,
        "qty": "10",
        "side": "buy",
        "status": "filled",
        "stop_price": null,
        "submitted_at": "2020-01-01T09:59:59Z",
        "symbol": "AAPL",
        "time_in_force": "day",
        "type": "market",
        "updated_at": "2020-01-01T10:00:01Z"
    })";

    Order order;
    Status status = order.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(order.asset_class, "us_equity");
    EXPECT_EQ(order.client_order_id, "client-456");
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.qty, "10");
    EXPECT_EQ(order.side, "buy");
    EXPECT_EQ(order.type, "market");
    EXPECT_EQ(order.status, "filled");
    EXPECT_FALSE(order.extended_hours);
}

TEST(OrderTest, FromJSONParseError) {
    Order order;
    Status status = order.fromJSON("invalid json");
    
    EXPECT_FALSE(status.ok());
}

TEST(OrderDirectionTest, ToStringConversion) {
    EXPECT_EQ(orderDirectionToString(OrderDirection::Ascending), "asc");
    EXPECT_EQ(orderDirectionToString(OrderDirection::Descending), "desc");
}

TEST(OrderSideTest, ToStringConversion) {
    EXPECT_EQ(orderSideToString(OrderSide::Buy), "buy");
    EXPECT_EQ(orderSideToString(OrderSide::Sell), "sell");
}

TEST(OrderTypeTest, ToStringConversion) {
    EXPECT_EQ(orderTypeToString(OrderType::Market), "market");
    EXPECT_EQ(orderTypeToString(OrderType::Limit), "limit");
    EXPECT_EQ(orderTypeToString(OrderType::Stop), "stop");
    EXPECT_EQ(orderTypeToString(OrderType::StopLimit), "stop_limit");
}

TEST(OrderTimeInForceTest, ToStringConversion) {
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::Day), "day");
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::GoodUntilCanceled), "gtc");
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::OPG), "opg");
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::CLS), "cls");
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::ImmediateOrCancel), "ioc");
    EXPECT_EQ(orderTimeInForceToString(OrderTimeInForce::FillOrKill), "fok");
}

TEST(OrderClassTest, ToStringConversion) {
    EXPECT_EQ(orderClassToString(OrderClass::Simple), "simple");
    EXPECT_EQ(orderClassToString(OrderClass::Bracket), "bracket");
    EXPECT_EQ(orderClassToString(OrderClass::OneCancelsOther), "oco");
    EXPECT_EQ(orderClassToString(OrderClass::OneTriggersOther), "oto");
}
