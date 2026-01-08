#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing portfolio history.
 */
class PortfolioHistory {
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
    double base_value = 0.0;
    std::vector<double> equity;
    std::vector<double> profit_loss;
    std::vector<double> profit_loss_pct;
    std::string timeframe;
    std::vector<uint64_t> timestamp;
};

}  // namespace alpaca::markets
