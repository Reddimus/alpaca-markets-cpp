#include <alpaca/markets/announcement.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

std::string announcementTypeToString(AnnouncementType type) {
    switch (type) {
        case AnnouncementType::Dividend:
            return "dividend";
        case AnnouncementType::Merger:
            return "merger";
        case AnnouncementType::Spinoff:
            return "spinoff";
        case AnnouncementType::Split:
            return "split";
        default:
            return "dividend";
    }
}

std::string announcementDateTypeToString(AnnouncementDateType type) {
    switch (type) {
        case AnnouncementDateType::DeclarationDate:
            return "declaration_date";
        case AnnouncementDateType::RecordDate:
            return "record_date";
        case AnnouncementDateType::ExDate:
            return "ex_date";
        case AnnouncementDateType::PayableDate:
            return "payable_date";
        default:
            return "declaration_date";
    }
}

Status Announcement::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing announcement JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an announcement object");
    }

    PARSE_STRING(id, "id")
    PARSE_STRING(corporate_actions_id, "corporate_actions_id")
    PARSE_STRING(ca_type, "ca_type")
    PARSE_STRING(ca_sub_type, "ca_sub_type")
    PARSE_STRING(initiating_symbol, "initiating_symbol")
    PARSE_STRING(initiating_original_cusip, "initiating_original_cusip")
    PARSE_STRING(target_symbol, "target_symbol")
    PARSE_STRING(target_original_cusip, "target_original_cusip")
    PARSE_STRING(declaration_date, "declaration_date")
    PARSE_STRING(expiration_date, "expiration_date")
    PARSE_STRING(record_date, "record_date")
    PARSE_STRING(payable_date, "payable_date")
    PARSE_STRING(cash, "cash")
    PARSE_STRING(old_rate, "old_rate")
    PARSE_STRING(new_rate, "new_rate")

    return Status();
}

}  // namespace alpaca::markets
