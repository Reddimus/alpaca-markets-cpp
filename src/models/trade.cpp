#include <alpaca/markets/trade.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Trade::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing trade JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a trade object");
    }

    // Market Data API v2 field names
    PARSE_DOUBLE(price, "p")           // price
    PARSE_UINT64(size, "s")            // size
    PARSE_STRING(exchange, "x")        // exchange
    PARSE_UINT64(id, "i")              // trade ID
    PARSE_STRING(timestamp, "t")       // timestamp
    PARSE_VECTOR_STRINGS(conditions, "c")  // conditions
    PARSE_STRING(tape, "z")            // tape

    return Status();
}

Status LatestTrade::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing latest trade JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a latest trade object");
    }

    PARSE_STRING(symbol, "symbol")

    // v2 API: trade is under "trade" key
    if (d.HasMember("trade") && d["trade"].IsObject()) {
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["trade"].Accept(writer);
        if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
