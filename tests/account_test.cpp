#include <alpaca/markets/account.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(AccountTest, FromJSON) {
    const std::string json = R"({
        "account_blocked": false,
        "account_number": "123456789",
        "buying_power": "100000.00",
        "cash": "50000.00",
        "created_at": "2020-01-01T00:00:00Z",
        "currency": "USD",
        "daytrade_count": 0,
        "daytrading_buying_power": "400000.00",
        "equity": "100000.00",
        "id": "test-id-123",
        "initial_margin": "0",
        "last_equity": "100000.00",
        "last_maintenance_margin": "0",
        "long_market_value": "0",
        "maintenance_margin": "0",
        "multiplier": "4",
        "pattern_day_trader": false,
        "portfolio_value": "100000.00",
        "regt_buying_power": "200000.00",
        "short_market_value": "0",
        "shorting_enabled": true,
        "sma": "0",
        "status": "ACTIVE",
        "trade_suspended_by_user": false,
        "trading_blocked": false,
        "transfers_blocked": false
    })";

    Account account;
    auto status = account.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(account.account_blocked);
    EXPECT_EQ(account.account_number, "123456789");
    EXPECT_EQ(account.buying_power, "100000.00");
    EXPECT_EQ(account.currency, "USD");
    EXPECT_EQ(account.id, "test-id-123");
    EXPECT_TRUE(account.shorting_enabled);
    EXPECT_EQ(account.status, "ACTIVE");
}

TEST(AccountTest, FromJSONParseError) {
    Account account;
    auto status = account.fromJSON("invalid json");
    
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.getCode(), 1);
}

TEST(AccountConfigurationsTest, FromJSON) {
    const std::string json = R"({
        "dtbp_check": "entry",
        "no_shorting": false,
        "suspend_trade": false,
        "trade_confirm_email": "all"
    })";

    AccountConfigurations config;
    auto status = config.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.dtbp_check, "entry");
    EXPECT_FALSE(config.no_shorting);
    EXPECT_FALSE(config.suspend_trade);
    EXPECT_EQ(config.trade_confirm_email, "all");
}

TEST(TradeActivityTest, FromJSON) {
    const std::string json = R"({
        "activity_type": "FILL",
        "cum_qty": "10",
        "id": "activity-123",
        "leaves_qty": "0",
        "order_id": "order-456",
        "price": "150.00",
        "qty": "10",
        "side": "buy",
        "symbol": "AAPL",
        "transaction_time": "2020-01-01T10:00:00Z",
        "type": "fill"
    })";

    TradeActivity activity;
    auto status = activity.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(activity.activity_type, "FILL");
    EXPECT_EQ(activity.symbol, "AAPL");
    EXPECT_EQ(activity.qty, "10");
    EXPECT_EQ(activity.price, "150.00");
}

TEST(NonTradeActivityTest, FromJSON) {
    const std::string json = R"({
        "activity_type": "DIV",
        "date": "2020-01-15",
        "id": "activity-789",
        "net_amount": "25.00",
        "per_share_amount": "0.25",
        "qty": "100",
        "symbol": "AAPL"
    })";

    NonTradeActivity activity;
    auto status = activity.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(activity.activity_type, "DIV");
    EXPECT_EQ(activity.symbol, "AAPL");
    EXPECT_EQ(activity.net_amount, "25.00");
}
