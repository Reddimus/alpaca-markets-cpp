#pragma once

#include <alpaca/markets/models/quote.hpp>

#include <map>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing multi-symbol historical quotes with pagination.
 * 
 * Used for batch retrieval of historical quotes across multiple symbols.
 */
class MultiQuotes {
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
    std::map<std::string, std::vector<Quote>> quotes;  // Symbol -> Quotes
    std::string next_page_token;                        // Pagination token
};

}  // namespace alpaca::markets
