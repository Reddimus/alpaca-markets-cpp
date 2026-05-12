#include <alpaca/markets/option.hpp>

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
    JSON_FROMJSON_PRELUDE("option contract");

    PARSE_STRING(id, "id");
    PARSE_STRING(symbol, "symbol");
    PARSE_STRING(name, "name");
    PARSE_BOOL(tradable, "tradable");
    PARSE_STRING(underlying_symbol, "underlying_symbol");
    PARSE_STRING(underlying_asset_id, "underlying_asset_id");
    PARSE_STRING(strike_price, "strike_price");
    PARSE_STRING(size, "size");
    PARSE_STRING(expiration_date, "expiration_date");
    PARSE_STRING(open_interest, "open_interest");
    PARSE_STRING(open_interest_date, "open_interest_date");
    PARSE_STRING(close_price, "close_price");
    PARSE_STRING(close_price_date, "close_price_date");

    // Parse status (string-typed enum)
    {
        std::string s;
        if (json_detail::obj_get_string(node, "status", s)) {
            status = stringToOptionStatus(s);
        }
    }

    // Parse type (call/put)
    {
        std::string s;
        if (json_detail::obj_get_string(node, "type", s)) {
            type = stringToOptionType(s);
        }
    }

    // Parse style (american/european)
    {
        std::string s;
        if (json_detail::obj_get_string(node, "style", s)) {
            style = stringToOptionStyle(s);
        }
    }

    // Parse deliverables array
    glz::generic::object_t::const_iterator del_it = node.find("deliverables");
    if (del_it != node.end() && del_it->second.is_array()) {
        for (const glz::generic& item : del_it->second.get_array()) {
            if (!item.is_object()) {
                continue;
            }
            const glz::generic::object_t& item_obj = item.get_object();
            Deliverable del;
            json_detail::obj_get_string(item_obj, "type", del.type);
            json_detail::obj_get_string(item_obj, "symbol", del.symbol);
            json_detail::obj_get_string(item_obj, "asset_id", del.asset_id);
            json_detail::obj_get_string(item_obj, "amount", del.amount);
            json_detail::obj_get_string(item_obj, "allocation_percentage", del.allocation_percentage);
            json_detail::obj_get_string(item_obj, "settlement_type", del.settlement_type);
            json_detail::obj_get_string(item_obj, "settlement_method", del.settlement_method);
            json_detail::obj_get_bool(item_obj, "delayed_settlement", del.delayed_settlement);
            deliverables.push_back(del);
        }
    }

    return Status();
}

Status OptionContracts::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("option contracts");

    // Parse option_contracts array
    glz::generic::object_t::const_iterator oc_it = node.find("option_contracts");
    if (oc_it != node.end() && oc_it->second.is_array()) {
        for (const glz::generic& item : oc_it->second.get_array()) {
            OptionContract contract;
            if (Status status = contract.fromJSON(json_detail::write(item)); !status.ok()) {
                return status;
            }
            option_contracts.push_back(contract);
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
