#include <alpaca/markets/clock.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Clock::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("clock");

    PARSE_BOOL(is_open, "is_open");
    PARSE_STRING(next_close, "next_close");
    PARSE_STRING(next_open, "next_open");
    PARSE_STRING(timestamp, "timestamp");

    return Status();
}

}  // namespace alpaca::markets
