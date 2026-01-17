#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief Option type (call or put)
 */
enum class OptionType {
    Call,
    Put,
};

/**
 * @brief Option style (american or european)
 */
enum class OptionStyle {
    American,
    European,
};

/**
 * @brief Option contract status
 */
enum class OptionStatus {
    Active,
    Inactive,
};

/**
 * @brief Deliverable associated with an option contract
 */
struct Deliverable {
    std::string type;           // "cash", "equity"
    std::string symbol;
    std::string asset_id;
    std::string amount;
    std::string allocation_percentage;
    std::string settlement_type;
    std::string settlement_method;
    bool delayed_settlement = false;
};

/**
 * @brief A type representing an Alpaca option contract.
 */
class OptionContract {
public:
    /**
     * @brief A method for deserializing JSON into the current object state.
     *
     * @param json The JSON string
     *
     * @return a Status indicating the success or failure of the operation.
     */
    Status fromJSON(const std::string& json);

public:
    std::string id;
    std::string symbol;
    std::string name;
    OptionStatus status = OptionStatus::Active;
    bool tradable = false;
    std::string underlying_symbol;
    std::string underlying_asset_id;
    OptionType type = OptionType::Call;
    OptionStyle style = OptionStyle::American;
    std::string strike_price;
    std::string size;               // multiplier, typically "100"
    std::string expiration_date;    // YYYY-MM-DD
    std::string open_interest;
    std::string open_interest_date;
    std::string close_price;
    std::string close_price_date;
    std::vector<Deliverable> deliverables;
};

/**
 * @brief Response containing multiple option contracts with pagination
 */
class OptionContracts {
public:
    /**
     * @brief A method for deserializing JSON into the current object state.
     *
     * @param json The JSON string
     *
     * @return a Status indicating the success or failure of the operation.
     */
    Status fromJSON(const std::string& json);

public:
    std::vector<OptionContract> option_contracts;
    std::string next_page_token;
};

// Conversion functions
std::string optionTypeToString(OptionType type);
OptionType stringToOptionType(const std::string& s);
std::string optionStyleToString(OptionStyle style);
OptionStyle stringToOptionStyle(const std::string& s);
std::string optionStatusToString(OptionStatus status);
OptionStatus stringToOptionStatus(const std::string& s);

}  // namespace alpaca::markets
