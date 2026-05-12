#include <alpaca/markets/calendar.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Date::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("calendar date");

    PARSE_STRING(close, "close");
    PARSE_STRING(date, "date");
    PARSE_STRING(open, "open");

    return Status();
}

}  // namespace alpaca::markets
