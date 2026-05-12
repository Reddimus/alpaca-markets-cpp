#include <alpaca/markets/corporate_action.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status CorporateAction::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("corporate action");

    PARSE_STRING(id, "id");
    PARSE_STRING(corporate_action_type, "ca_type");
    PARSE_STRING(symbol, "symbol");
    PARSE_STRING(new_symbol, "new_symbol");
    PARSE_STRING(description, "description");
    PARSE_STRING(process_date, "process_date");
    PARSE_STRING(ex_date, "ex_date");
    PARSE_STRING(record_date, "record_date");
    PARSE_STRING(payable_date, "payable_date");
    PARSE_DOUBLE(old_rate, "old_rate");
    PARSE_DOUBLE(new_rate, "new_rate");
    PARSE_DOUBLE(rate, "rate");
    PARSE_DOUBLE(cash, "cash");
    PARSE_STRING(created_at, "created_at");
    PARSE_STRING(updated_at, "updated_at");

    return Status();
}

Status CorporateActions::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("corporate actions");

    // Parse corporate_actions array
    glz::generic::object_t::const_iterator ca_it = node.find("corporate_actions");
    if (ca_it != node.end() && ca_it->second.is_array()) {
        for (const glz::generic& item : ca_it->second.get_array()) {
            CorporateAction action;
            if (Status status = action.fromJSON(json_detail::write(item)); !status.ok()) {
                return status;
            }
            corporate_actions.push_back(action);
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
