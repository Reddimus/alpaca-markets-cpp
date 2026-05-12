#include <alpaca/markets/multi_trade.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status MultiTrades::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("multi trades");

    // Parse trades map
    glz::generic::object_t::const_iterator trades_it = node.find("trades");
    if (trades_it != node.end() && trades_it->second.is_object()) {
        for (const auto& [symbol, value] : trades_it->second.get_object()) {
            std::vector<Trade> symbol_trades;
            if (value.is_array()) {
                for (const glz::generic& item : value.get_array()) {
                    Trade trade;
                    if (Status status = trade.fromJSON(json_detail::write(item)); !status.ok()) {
                        return status;
                    }
                    symbol_trades.push_back(trade);
                }
            }
            trades[symbol] = symbol_trades;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
