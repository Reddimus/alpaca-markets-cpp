#include <alpaca/markets/auction.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Auction::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("auction");

    PARSE_STRING(timestamp, "t");
    PARSE_DOUBLE(price, "p");
    PARSE_UINT64(size, "s");
    PARSE_STRING(exchange, "x");
    PARSE_STRING(condition, "c");

    return Status();
}

Status SymbolAuctions::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("symbol auctions");

    // Parse daily auctions array "d"
    glz::generic::object_t::const_iterator d_it = node.find("d");
    if (d_it != node.end() && d_it->second.is_array()) {
        for (const glz::generic& elem : d_it->second.get_array()) {
            Auction auction;
            if (Status status = auction.fromJSON(json_detail::write(elem)); !status.ok()) {
                return status;
            }
            daily_auctions.push_back(auction);
        }
    }

    return Status();
}

Status Auctions::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("auctions");

    // Parse auctions map
    glz::generic::object_t::const_iterator auctions_it = node.find("auctions");
    if (auctions_it != node.end() && auctions_it->second.is_object()) {
        for (const auto& [symbol, value] : auctions_it->second.get_object()) {
            SymbolAuctions symbol_auctions;
            if (Status status = symbol_auctions.fromJSON(json_detail::write(value)); !status.ok()) {
                return status;
            }
            auctions[symbol] = symbol_auctions;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
