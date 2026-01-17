#include <alpaca/markets/option.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

std::string optionTypeToString(OptionType type) {
    switch (type) {
        case OptionType::Call:
            return "call";
        case OptionType::Put:
            return "put";
        default:
            return "call";
    }
}

OptionType stringToOptionType(const std::string& s) {
    if (s == "put") {
        return OptionType::Put;
    }
    return OptionType::Call;
}

std::string optionStyleToString(OptionStyle style) {
    switch (style) {
        case OptionStyle::American:
            return "american";
        case OptionStyle::European:
            return "european";
        default:
            return "american";
    }
}

OptionStyle stringToOptionStyle(const std::string& s) {
    if (s == "european") {
        return OptionStyle::European;
    }
    return OptionStyle::American;
}

std::string optionStatusToString(OptionStatus status) {
    switch (status) {
        case OptionStatus::Active:
            return "active";
        case OptionStatus::Inactive:
            return "inactive";
        default:
            return "active";
    }
}

OptionStatus stringToOptionStatus(const std::string& s) {
    if (s == "inactive") {
        return OptionStatus::Inactive;
    }
    return OptionStatus::Active;
}

Status OptionContract::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing option contract JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an option contract object");
    }

    PARSE_STRING(id, "id")
    PARSE_STRING(symbol, "symbol")
    PARSE_STRING(name, "name")
    PARSE_BOOL(tradable, "tradable")
    PARSE_STRING(underlying_symbol, "underlying_symbol")
    PARSE_STRING(underlying_asset_id, "underlying_asset_id")
    PARSE_STRING(strike_price, "strike_price")
    PARSE_STRING(size, "size")
    PARSE_STRING(expiration_date, "expiration_date")
    PARSE_STRING(open_interest, "open_interest")
    PARSE_STRING(open_interest_date, "open_interest_date")
    PARSE_STRING(close_price, "close_price")
    PARSE_STRING(close_price_date, "close_price_date")

    // Parse status
    if (d.HasMember("status") && d["status"].IsString()) {
        status = stringToOptionStatus(d["status"].GetString());
    }

    // Parse type (call/put)
    if (d.HasMember("type") && d["type"].IsString()) {
        type = stringToOptionType(d["type"].GetString());
    }

    // Parse style (american/european)
    if (d.HasMember("style") && d["style"].IsString()) {
        style = stringToOptionStyle(d["style"].GetString());
    }

    // Parse deliverables array
    if (d.HasMember("deliverables") && d["deliverables"].IsArray()) {
        for (auto& item : d["deliverables"].GetArray()) {
            if (item.IsObject()) {
                Deliverable del;
                if (item.HasMember("type") && item["type"].IsString()) {
                    del.type = item["type"].GetString();
                }
                if (item.HasMember("symbol") && item["symbol"].IsString()) {
                    del.symbol = item["symbol"].GetString();
                }
                if (item.HasMember("asset_id") && item["asset_id"].IsString()) {
                    del.asset_id = item["asset_id"].GetString();
                }
                if (item.HasMember("amount") && item["amount"].IsString()) {
                    del.amount = item["amount"].GetString();
                }
                if (item.HasMember("allocation_percentage") && item["allocation_percentage"].IsString()) {
                    del.allocation_percentage = item["allocation_percentage"].GetString();
                }
                if (item.HasMember("settlement_type") && item["settlement_type"].IsString()) {
                    del.settlement_type = item["settlement_type"].GetString();
                }
                if (item.HasMember("settlement_method") && item["settlement_method"].IsString()) {
                    del.settlement_method = item["settlement_method"].GetString();
                }
                if (item.HasMember("delayed_settlement") && item["delayed_settlement"].IsBool()) {
                    del.delayed_settlement = item["delayed_settlement"].GetBool();
                }
                deliverables.push_back(del);
            }
        }
    }

    return Status();
}

Status OptionContracts::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing option contracts JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an option contracts object");
    }

    // Parse option_contracts array
    if (d.HasMember("option_contracts") && d["option_contracts"].IsArray()) {
        for (auto& item : d["option_contracts"].GetArray()) {
            OptionContract contract;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            item.Accept(writer);
            if (Status status = contract.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            option_contracts.push_back(contract);
        }
    }

    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

}  // namespace alpaca::markets
