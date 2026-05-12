#include <alpaca/markets/watchlist.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Watchlist::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("watchlist");

    PARSE_STRING(account_id, "account_id");
    PARSE_STRING(created_at, "created_at");
    PARSE_STRING(id, "id");
    PARSE_STRING(name, "name");
    PARSE_STRING(updated_at, "updated_at");

    // Parse assets array
    glz::generic::object_t::const_iterator assets_it = node.find("assets");
    if (assets_it != node.end() && assets_it->second.is_array()) {
        assets.clear();
        for (const glz::generic& item : assets_it->second.get_array()) {
            Asset asset;
            if (Status status = asset.fromJSON(json_detail::write(item)); !status.ok()) {
                return status;
            }
            assets.push_back(asset);
        }
    }

    return Status();
}

}  // namespace alpaca::markets
