#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing a single auction entry.
 * 
 * Auctions represent opening and closing auction data for a stock.
 */
class Auction {
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
    std::string timestamp;     // "t" - ISO 8601 timestamp
    double price = 0.0;        // "p" - auction price
    uint64_t size = 0;         // "s" - auction size
    std::string exchange;      // "x" - exchange code
    std::string condition;     // "c" - condition code
};

/**
 * @brief A type representing auction data for a symbol with opening and closing auctions.
 */
class SymbolAuctions {
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
    std::vector<Auction> daily_auctions;   // "d" - daily auctions (opening/closing)
};

/**
 * @brief A type representing auctions for multiple symbols with pagination.
 */
class Auctions {
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
    std::map<std::string, SymbolAuctions> auctions;  // Symbol -> SymbolAuctions
    std::string next_page_token;                      // Pagination token
};

}  // namespace alpaca::markets
