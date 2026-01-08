#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>

namespace alpaca::markets {

/**
 * @brief A type representing a calendar day.
 */
class Date {
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
    std::string close;
    std::string date;
    std::string open;
};

}  // namespace alpaca::markets
