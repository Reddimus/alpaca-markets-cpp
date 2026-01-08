#include <alpaca/markets/calendar.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Date::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing calendar date JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a calendar date object");
    }

    PARSE_STRING(close, "close")
    PARSE_STRING(date, "date")
    PARSE_STRING(open, "open")

    return Status();
}

}  // namespace alpaca::markets
