#include <gtest/gtest.h>

#include <alpaca/markets/corporate_action.hpp>

using namespace alpaca::markets;

TEST(CorporateActionTest, FromJSON) {
    CorporateAction action;
    std::string json = R"({
        "id": "ca123",
        "ca_type": "forward_split",
        "symbol": "AAPL",
        "description": "4-for-1 stock split",
        "process_date": "2024-08-01",
        "ex_date": "2024-08-05",
        "record_date": "2024-07-31",
        "payable_date": "2024-08-05",
        "old_rate": 1.0,
        "new_rate": 4.0,
        "rate": 4.0,
        "created_at": "2024-07-15T10:00:00Z",
        "updated_at": "2024-07-15T10:00:00Z"
    })";

    ASSERT_TRUE(action.fromJSON(json).ok());
    EXPECT_EQ(action.id, "ca123");
    EXPECT_EQ(action.corporate_action_type, "forward_split");
    EXPECT_EQ(action.symbol, "AAPL");
    EXPECT_EQ(action.description, "4-for-1 stock split");
    EXPECT_EQ(action.process_date, "2024-08-01");
    EXPECT_EQ(action.ex_date, "2024-08-05");
    EXPECT_DOUBLE_EQ(action.old_rate, 1.0);
    EXPECT_DOUBLE_EQ(action.new_rate, 4.0);
    EXPECT_DOUBLE_EQ(action.rate, 4.0);
}

TEST(CorporateActionTest, FromJSONParseError) {
    CorporateAction action;
    std::string json = "not valid json";

    EXPECT_FALSE(action.fromJSON(json).ok());
}

TEST(CorporateActionTest, CashDividend) {
    CorporateAction action;
    std::string json = R"({
        "id": "div456",
        "ca_type": "cash_dividend",
        "symbol": "MSFT",
        "description": "Quarterly dividend",
        "ex_date": "2024-02-14",
        "record_date": "2024-02-15",
        "payable_date": "2024-03-14",
        "cash": 0.75
    })";

    ASSERT_TRUE(action.fromJSON(json).ok());
    EXPECT_EQ(action.id, "div456");
    EXPECT_EQ(action.corporate_action_type, "cash_dividend");
    EXPECT_EQ(action.symbol, "MSFT");
    EXPECT_DOUBLE_EQ(action.cash, 0.75);
}

TEST(CorporateActionTest, NameChange) {
    CorporateAction action;
    std::string json = R"({
        "id": "nc789",
        "ca_type": "name_change",
        "symbol": "FB",
        "new_symbol": "META",
        "description": "Symbol change from FB to META"
    })";

    ASSERT_TRUE(action.fromJSON(json).ok());
    EXPECT_EQ(action.corporate_action_type, "name_change");
    EXPECT_EQ(action.symbol, "FB");
    EXPECT_EQ(action.new_symbol, "META");
}

TEST(CorporateActionsTest, FromJSON) {
    CorporateActions actions;
    std::string json = R"({
        "corporate_actions": [
            {
                "id": "ca1",
                "ca_type": "forward_split",
                "symbol": "AAPL",
                "rate": 4.0
            },
            {
                "id": "ca2",
                "ca_type": "cash_dividend",
                "symbol": "MSFT",
                "cash": 0.75
            }
        ],
        "next_page_token": "next123"
    })";

    ASSERT_TRUE(actions.fromJSON(json).ok());
    EXPECT_EQ(actions.corporate_actions.size(), 2);
    EXPECT_EQ(actions.corporate_actions[0].id, "ca1");
    EXPECT_EQ(actions.corporate_actions[0].corporate_action_type, "forward_split");
    EXPECT_EQ(actions.corporate_actions[1].id, "ca2");
    EXPECT_EQ(actions.corporate_actions[1].corporate_action_type, "cash_dividend");
    EXPECT_EQ(actions.next_page_token, "next123");
}
