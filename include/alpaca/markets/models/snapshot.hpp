#pragma once

#include <alpaca/markets/models/bars.hpp>
#include <alpaca/markets/models/quote.hpp>
#include <alpaca/markets/models/status.hpp>
#include <alpaca/markets/models/trade.hpp>

#include <map>
#include <string>

namespace alpaca::markets {

/**
 * @brief A type representing a market snapshot for a symbol.
 * 
 * Contains the latest trade, quote, minute bar, daily bar, and previous daily bar.
 */
class Snapshot {
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
    Trade latest_trade;
    Quote latest_quote;
    Bar minute_bar;
    Bar daily_bar;
    Bar prev_daily_bar;
};

/**
 * @brief Response containing snapshots for multiple symbols
 */
class Snapshots {
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
    std::map<std::string, Snapshot> snapshots;
};

}  // namespace alpaca::markets
