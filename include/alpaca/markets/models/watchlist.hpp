#pragma once

#include <alpaca/markets/models/asset.hpp>
#include <alpaca/markets/models/status.hpp>

#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A type representing an Alpaca watchlist
 */
class Watchlist {
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
    std::string account_id;
    std::vector<Asset> assets;
    std::string created_at;
    std::string id;
    std::string name;
    std::string updated_at;
};

}  // namespace alpaca::markets
