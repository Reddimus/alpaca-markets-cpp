#include <alpaca/markets/auction.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Auction::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing auction JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an auction object");
    }

    PARSE_STRING(timestamp, "t")
    PARSE_DOUBLE(price, "p")
    PARSE_UINT64(size, "s")
    PARSE_STRING(exchange, "x")
    PARSE_STRING(condition, "c")

    return Status();
}

Status SymbolAuctions::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing symbol auctions JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a symbol auctions object");
    }

    // Parse daily auctions array "d"
    if (d.HasMember("d") && d["d"].IsArray()) {
        for (auto& o : d["d"].GetArray()) {
            Auction auction;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            o.Accept(writer);
            if (Status status = auction.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            daily_auctions.push_back(auction);
        }
    }

    return Status();
}

Status Auctions::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing auctions JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an auctions object");
    }

    // Parse auctions map
    if (d.HasMember("auctions") && d["auctions"].IsObject()) {
        for (auto& m : d["auctions"].GetObject()) {
            SymbolAuctions symbol_auctions;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = symbol_auctions.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            auctions[m.name.GetString()] = symbol_auctions;
        }
    }

    // Parse next_page_token
    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return Status();
}

}  // namespace alpaca::markets
