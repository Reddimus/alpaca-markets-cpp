#include <alpaca/markets/option.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(OptionContractTest, FromJSON) {
    const std::string json = R"({
        "id": "c1234567-89ab-cdef-0123-456789abcdef",
        "symbol": "AAPL241220C00200000",
        "name": "AAPL Dec 20 2024 200 Call",
        "status": "active",
        "tradable": true,
        "underlying_symbol": "AAPL",
        "underlying_asset_id": "a1234567-89ab-cdef-0123-456789abcdef",
        "type": "call",
        "style": "american",
        "strike_price": "200.00",
        "size": "100",
        "expiration_date": "2024-12-20",
        "open_interest": "1500",
        "open_interest_date": "2024-01-10",
        "close_price": "5.25",
        "close_price_date": "2024-01-10"
    })";

    OptionContract contract;
    Status status = contract.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(contract.id, "c1234567-89ab-cdef-0123-456789abcdef");
    EXPECT_EQ(contract.symbol, "AAPL241220C00200000");
    EXPECT_EQ(contract.name, "AAPL Dec 20 2024 200 Call");
    EXPECT_EQ(contract.status, OptionStatus::Active);
    EXPECT_TRUE(contract.tradable);
    EXPECT_EQ(contract.underlying_symbol, "AAPL");
    EXPECT_EQ(contract.type, OptionType::Call);
    EXPECT_EQ(contract.style, OptionStyle::American);
    EXPECT_EQ(contract.strike_price, "200.00");
    EXPECT_EQ(contract.size, "100");
    EXPECT_EQ(contract.expiration_date, "2024-12-20");
    EXPECT_EQ(contract.open_interest, "1500");
    EXPECT_EQ(contract.close_price, "5.25");
}

TEST(OptionContractTest, FromJSONPutOption) {
    const std::string json = R"({
        "id": "p1234567-89ab-cdef-0123-456789abcdef",
        "symbol": "AAPL241220P00180000",
        "name": "AAPL Dec 20 2024 180 Put",
        "status": "active",
        "tradable": true,
        "underlying_symbol": "AAPL",
        "type": "put",
        "style": "american",
        "strike_price": "180.00",
        "size": "100",
        "expiration_date": "2024-12-20"
    })";

    OptionContract contract;
    Status status = contract.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(contract.type, OptionType::Put);
}

TEST(OptionContractTest, FromJSONWithDeliverables) {
    const std::string json = R"({
        "id": "d1234567-89ab-cdef-0123-456789abcdef",
        "symbol": "AAPL241220C00200000",
        "type": "call",
        "style": "american",
        "deliverables": [
            {
                "type": "equity",
                "symbol": "AAPL",
                "asset_id": "asset123",
                "amount": "100",
                "allocation_percentage": "100",
                "settlement_type": "T+1",
                "settlement_method": "delivery",
                "delayed_settlement": false
            }
        ]
    })";

    OptionContract contract;
    Status status = contract.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(contract.deliverables.size(), 1u);
    EXPECT_EQ(contract.deliverables[0].type, "equity");
    EXPECT_EQ(contract.deliverables[0].symbol, "AAPL");
    EXPECT_EQ(contract.deliverables[0].amount, "100");
    EXPECT_FALSE(contract.deliverables[0].delayed_settlement);
}

TEST(OptionContractsTest, FromJSON) {
    const std::string json = R"({
        "option_contracts": [
            {
                "id": "c1",
                "symbol": "AAPL241220C00200000",
                "type": "call",
                "style": "american",
                "strike_price": "200.00"
            },
            {
                "id": "c2",
                "symbol": "AAPL241220P00180000",
                "type": "put",
                "style": "american",
                "strike_price": "180.00"
            }
        ],
        "next_page_token": "page2token"
    })";

    OptionContracts contracts;
    Status status = contracts.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(contracts.option_contracts.size(), 2u);
    EXPECT_EQ(contracts.option_contracts[0].symbol, "AAPL241220C00200000");
    EXPECT_EQ(contracts.option_contracts[0].type, OptionType::Call);
    EXPECT_EQ(contracts.option_contracts[1].symbol, "AAPL241220P00180000");
    EXPECT_EQ(contracts.option_contracts[1].type, OptionType::Put);
    EXPECT_EQ(contracts.next_page_token, "page2token");
}

TEST(OptionTypeTest, Conversions) {
    EXPECT_EQ(optionTypeToString(OptionType::Call), "call");
    EXPECT_EQ(optionTypeToString(OptionType::Put), "put");
    
    EXPECT_EQ(stringToOptionType("call"), OptionType::Call);
    EXPECT_EQ(stringToOptionType("put"), OptionType::Put);
    EXPECT_EQ(stringToOptionType("unknown"), OptionType::Call);  // default
}

TEST(OptionStyleTest, Conversions) {
    EXPECT_EQ(optionStyleToString(OptionStyle::American), "american");
    EXPECT_EQ(optionStyleToString(OptionStyle::European), "european");
    
    EXPECT_EQ(stringToOptionStyle("american"), OptionStyle::American);
    EXPECT_EQ(stringToOptionStyle("european"), OptionStyle::European);
    EXPECT_EQ(stringToOptionStyle("unknown"), OptionStyle::American);  // default
}

TEST(OptionStatusTest, Conversions) {
    EXPECT_EQ(optionStatusToString(OptionStatus::Active), "active");
    EXPECT_EQ(optionStatusToString(OptionStatus::Inactive), "inactive");
    
    EXPECT_EQ(stringToOptionStatus("active"), OptionStatus::Active);
    EXPECT_EQ(stringToOptionStatus("inactive"), OptionStatus::Inactive);
    EXPECT_EQ(stringToOptionStatus("unknown"), OptionStatus::Active);  // default
}

TEST(OptionContractTest, FromJSONParseError) {
    OptionContract contract;
    Status status = contract.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
