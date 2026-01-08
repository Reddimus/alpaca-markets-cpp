#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing a quote.
 * 
 * Updated for Market Data API v2 response format.
 */
class Quote {
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
    double ask_price = 0.0;      // "ap"
    uint64_t ask_size = 0;       // "as"
    std::string ask_exchange;    // "ax"
    double bid_price = 0.0;      // "bp"
    uint64_t bid_size = 0;       // "bs"
    std::string bid_exchange;    // "bx"
    std::string timestamp;       // "t" - ISO 8601
    std::vector<std::string> conditions;  // "c" - quote conditions
};

/**
 * @brief A type representing the latest quote for a certain symbol.
 * 
 * Updated for Market Data API v2 response format.
 */
class LatestQuote {
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
    std::string symbol;
    Quote quote;
};

// Legacy alias for backward compatibility
using LastQuote = LatestQuote;

}  // namespace alpaca::markets
