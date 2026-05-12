#include <alpaca/markets/portfolio.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status PortfolioHistory::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("portfolio history");

    PARSE_DOUBLE(base_value, "base_value");
    PARSE_VECTOR_DOUBLES(equity, "equity");
    PARSE_VECTOR_DOUBLES(profit_loss, "profit_loss");
    PARSE_VECTOR_DOUBLES(profit_loss_pct, "profit_loss_pct");
    PARSE_STRING(timeframe, "timeframe");
    PARSE_VECTOR_UINT64(timestamp, "timestamp");

    return Status();
}

}  // namespace alpaca::markets
