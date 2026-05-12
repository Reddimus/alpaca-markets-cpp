#include <alpaca/markets/trade.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Trade::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("trade");

    // Market Data API v2 field names
    PARSE_DOUBLE(price, "p");               // price
    PARSE_UINT64(size, "s");                // size
    PARSE_STRING(exchange, "x");            // exchange
    PARSE_UINT64(id, "i");                  // trade ID
    PARSE_STRING(timestamp, "t");           // timestamp
    PARSE_VECTOR_STRINGS(conditions, "c");  // conditions
    PARSE_STRING(tape, "z");                // tape

    return Status();
}

Status LatestTrade::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("latest trade");

    PARSE_STRING(symbol, "symbol");

    // v2 API: trade is under "trade" key
    glz::generic::object_t::const_iterator trade_it = node.find("trade");
    if (trade_it != node.end() && trade_it->second.is_object()) {
        if (Status status = trade.fromJSON(json_detail::write(trade_it->second)); !status.ok()) {
            return status;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
