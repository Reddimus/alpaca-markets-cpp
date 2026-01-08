#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing an Alpaca bar (OHLCV data).
 * 
 * Updated for Market Data API v2 response format.
 */
class Bar {
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
    std::string timestamp;  // v2 uses ISO 8601 timestamp string "t"
    double open_price = 0.0;   // "o"
    double high_price = 0.0;   // "h"
    double low_price = 0.0;    // "l"
    double close_price = 0.0;  // "c"
    uint64_t volume = 0;       // "v"
    uint64_t trade_count = 0;  // "n" (number of trades)
    double vwap = 0.0;         // "vw" (volume weighted average price)
};

/**
 * @brief A type representing bars for multiple symbols
 */
class Bars {
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
    std::map<std::string, std::vector<Bar>> bars;
    std::string next_page_token;  // v2 pagination
};

}  // namespace alpaca::markets
