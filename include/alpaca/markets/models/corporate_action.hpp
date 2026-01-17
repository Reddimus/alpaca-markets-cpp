#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing a corporate action event from Market Data API.
 * 
 * This is distinct from Trading API announcements - it represents market data
 * corporate actions like symbol changes, stock dividends, cash dividends, splits, etc.
 */
class CorporateAction {
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
    std::string id;                    // Corporate action ID
    std::string corporate_action_type; // Type: reverse_split, forward_split, unit_split, 
                                       // cash_dividend, stock_dividend, spin_off, cash_merger,
                                       // stock_merger, stock_and_cash_merger, redemption,
                                       // name_change, worthless_removal, rights_distribution
    std::string symbol;                // Symbol
    std::string new_symbol;            // New symbol (for symbol changes)
    std::string description;           // Description
    std::string process_date;          // Process date (YYYY-MM-DD)
    std::string ex_date;               // Ex-date (YYYY-MM-DD)
    std::string record_date;           // Record date
    std::string payable_date;          // Payable date
    
    // Split/dividend details
    double old_rate = 0.0;             // Old rate (for splits)
    double new_rate = 0.0;             // New rate (for splits)
    double rate = 0.0;                 // Dividend rate or split ratio
    double cash = 0.0;                 // Cash amount (for cash dividends/mergers)
    
    // Created/updated timestamps
    std::string created_at;
    std::string updated_at;
};

/**
 * @brief A type representing corporate actions with pagination.
 */
class CorporateActions {
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
    std::vector<CorporateAction> corporate_actions;
    std::string next_page_token;
};

}  // namespace alpaca::markets
