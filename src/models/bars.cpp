#include <alpaca/markets/bars.hpp>

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Bar::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("bar");

    // Market Data API v2 field names
    PARSE_STRING(timestamp, "t");    // timestamp
    PARSE_DOUBLE(open_price, "o");   // open
    PARSE_DOUBLE(high_price, "h");   // high
    PARSE_DOUBLE(low_price, "l");    // low
    PARSE_DOUBLE(close_price, "c");  // close
    PARSE_UINT64(volume, "v");       // volume
    PARSE_UINT64(trade_count, "n");  // number of trades
    PARSE_DOUBLE(vwap, "vw");        // volume weighted average price

    return Status();
}

Status Bars::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("bars");

    // v2 API: bars are under "bars" key, keyed by symbol
    glz::generic::object_t::const_iterator bars_it = node.find("bars");
    if (bars_it != node.end() && bars_it->second.is_object()) {
        for (const auto& [symbol, value] : bars_it->second.get_object()) {
            std::vector<Bar> symbol_bars;
            if (value.is_array()) {
                for (const glz::generic& b : value.get_array()) {
                    Bar bar;
                    if (Status status = bar.fromJSON(json_detail::write(b)); !status.ok()) {
                        return status;
                    }
                    symbol_bars.push_back(bar);
                }
            }
            bars[symbol] = symbol_bars;
        }
    }

    // Pagination token
    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
