#include <alpaca/markets/quote.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Quote::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing quote JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a quote object");
    }

    // Market Data API v2 field names
    PARSE_DOUBLE(ask_price, "ap")     // ask price
    PARSE_UINT64(ask_size, "as")      // ask size
    PARSE_STRING(ask_exchange, "ax")  // ask exchange
    PARSE_DOUBLE(bid_price, "bp")     // bid price
    PARSE_UINT64(bid_size, "bs")      // bid size
    PARSE_STRING(bid_exchange, "bx")  // bid exchange
    PARSE_STRING(timestamp, "t")      // timestamp
    PARSE_VECTOR_STRINGS(conditions, "c")  // conditions

    return Status();
}

Status LatestQuote::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing latest quote JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a latest quote object");
    }

    PARSE_STRING(symbol, "symbol")

    // v2 API: quote is under "quote" key
    if (d.HasMember("quote") && d["quote"].IsObject()) {
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["quote"].Accept(writer);
        if (auto status = quote.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
