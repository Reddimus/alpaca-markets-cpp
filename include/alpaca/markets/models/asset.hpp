#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>

namespace alpaca::markets {

/**
 * @brief The class of an asset.
 */
enum class AssetClass {
    USEquity,
};

/**
 * @brief A helper to convert an AssetClass to a string
 */
std::string assetClassToString(AssetClass asset_class);

/**
 * @brief A type representing an Alpaca asset.
 */
class Asset {
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
    std::string asset_class;
    bool easy_to_borrow = false;
    std::string exchange;
    std::string id;
    bool marginable = false;
    bool shortable = false;
    std::string status;
    std::string symbol;
    bool tradable = false;
};

}  // namespace alpaca::markets
