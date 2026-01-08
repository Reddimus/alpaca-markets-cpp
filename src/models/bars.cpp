#include <alpaca/markets/bars.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Bar::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing bar JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a bar object");
    }

    // Market Data API v2 field names
    PARSE_STRING(timestamp, "t")      // timestamp
    PARSE_DOUBLE(open_price, "o")     // open
    PARSE_DOUBLE(high_price, "h")     // high
    PARSE_DOUBLE(low_price, "l")      // low
    PARSE_DOUBLE(close_price, "c")    // close
    PARSE_UINT64(volume, "v")         // volume
    PARSE_UINT64(trade_count, "n")    // number of trades
    PARSE_DOUBLE(vwap, "vw")          // volume weighted average price

    return Status();
}

Status Bars::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing bars JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a bars object");
    }

    // v2 API: bars are under "bars" key, keyed by symbol
    if (d.HasMember("bars") && d["bars"].IsObject()) {
        for (auto& m : d["bars"].GetObject()) {
            std::string symbol = m.name.GetString();
            std::vector<Bar> symbol_bars;

            if (m.value.IsArray()) {
                for (auto& b : m.value.GetArray()) {
                    Bar bar;
                    rapidjson::StringBuffer s;
                    s.Clear();
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    b.Accept(writer);
                    if (Status status = bar.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_bars.push_back(bar);
                }
            }
            bars[symbol] = symbol_bars;
        }
    }

    // Pagination token
    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

}  // namespace alpaca::markets
