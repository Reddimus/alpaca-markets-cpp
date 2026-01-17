#include <alpaca/markets/multi_trade.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status MultiTrades::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing multi trades JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a multi trades object");
    }

    // Parse trades map
    if (d.HasMember("trades") && d["trades"].IsObject()) {
        for (auto& m : d["trades"].GetObject()) {
            std::vector<Trade> symbol_trades;
            if (m.value.IsArray()) {
                for (auto& o : m.value.GetArray()) {
                    Trade trade;
                    rapidjson::StringBuffer s;
                    s.Clear();
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    o.Accept(writer);
                    if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_trades.push_back(trade);
                }
            }
            trades[m.name.GetString()] = symbol_trades;
        }
    }

    // Parse next_page_token
    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return Status();
}

}  // namespace alpaca::markets
