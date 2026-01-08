#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing a trade.
 * 
 * Updated for Market Data API v2 response format.
 */
class Trade {
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
    double price = 0.0;           // "p"
    uint64_t size = 0;            // "s"
    std::string exchange;         // "x"
    uint64_t id = 0;              // "i" - trade ID
    std::string timestamp;        // "t" - ISO 8601
    std::vector<std::string> conditions;  // "c" - trade conditions
    std::string tape;             // "z" - tape (A, B, or C)
};

/**
 * @brief A type representing the latest trade for a certain symbol.
 * 
 * Updated for Market Data API v2 response format.
 */
class LatestTrade {
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
    Trade trade;
};

// Legacy alias for backward compatibility
using LastTrade = LatestTrade;

}  // namespace alpaca::markets
