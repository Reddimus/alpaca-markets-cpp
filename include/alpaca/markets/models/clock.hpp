#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>

namespace alpaca::markets {

/**
 * @brief A type representing the market clock.
 */
class Clock {
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
    bool is_open = false;
    std::string next_close;
    std::string next_open;
    std::string timestamp;
};

}  // namespace alpaca::markets
