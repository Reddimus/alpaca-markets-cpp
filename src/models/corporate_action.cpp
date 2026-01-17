#include <alpaca/markets/corporate_action.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status CorporateAction::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing corporate action JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a corporate action object");
    }

    PARSE_STRING(id, "id")
    PARSE_STRING(corporate_action_type, "ca_type")
    PARSE_STRING(symbol, "symbol")
    PARSE_STRING(new_symbol, "new_symbol")
    PARSE_STRING(description, "description")
    PARSE_STRING(process_date, "process_date")
    PARSE_STRING(ex_date, "ex_date")
    PARSE_STRING(record_date, "record_date")
    PARSE_STRING(payable_date, "payable_date")
    PARSE_DOUBLE(old_rate, "old_rate")
    PARSE_DOUBLE(new_rate, "new_rate")
    PARSE_DOUBLE(rate, "rate")
    PARSE_DOUBLE(cash, "cash")
    PARSE_STRING(created_at, "created_at")
    PARSE_STRING(updated_at, "updated_at")

    return Status();
}

Status CorporateActions::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing corporate actions JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a corporate actions object");
    }

    // Parse corporate_actions array
    if (d.HasMember("corporate_actions") && d["corporate_actions"].IsArray()) {
        for (auto& o : d["corporate_actions"].GetArray()) {
            CorporateAction action;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            o.Accept(writer);
            if (Status status = action.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            corporate_actions.push_back(action);
        }
    }

    // Parse next_page_token
    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return Status();
}

}  // namespace alpaca::markets
