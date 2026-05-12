#include <alpaca/markets/quote.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Quote::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("quote");

    // Market Data API v2 field names
    PARSE_DOUBLE(ask_price, "ap");          // ask price
    PARSE_UINT64(ask_size, "as");           // ask size
    PARSE_STRING(ask_exchange, "ax");       // ask exchange
    PARSE_DOUBLE(bid_price, "bp");          // bid price
    PARSE_UINT64(bid_size, "bs");           // bid size
    PARSE_STRING(bid_exchange, "bx");       // bid exchange
    PARSE_STRING(timestamp, "t");           // timestamp
    PARSE_VECTOR_STRINGS(conditions, "c");  // conditions

    return Status();
}

Status LatestQuote::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("latest quote");

    PARSE_STRING(symbol, "symbol");

    // v2 API: quote is under "quote" key
    glz::generic::object_t::const_iterator quote_it = node.find("quote");
    if (quote_it != node.end() && quote_it->second.is_object()) {
        if (Status status = quote.fromJSON(json_detail::write(quote_it->second)); !status.ok()) {
            return status;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
