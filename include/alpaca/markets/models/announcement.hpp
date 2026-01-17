#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief Types of corporate action announcements
 */
enum class AnnouncementType {
    Dividend,
    Merger,
    Spinoff,
    Split,
};

/**
 * @brief A helper to convert an AnnouncementType to a string
 */
std::string announcementTypeToString(AnnouncementType type);

/**
 * @brief Date type for corporate action queries
 */
enum class AnnouncementDateType {
    DeclarationDate,
    RecordDate,
    ExDate,
    PayableDate,
};

/**
 * @brief A helper to convert an AnnouncementDateType to a string
 */
std::string announcementDateTypeToString(AnnouncementDateType type);

/**
 * @brief A type representing an Alpaca corporate action announcement.
 *
 * Corporate actions include dividends, mergers, spinoffs, and stock splits.
 * See: https://alpaca.markets/docs/api-references/trading-api/corporate-actions-announcements/
 */
class Announcement {
public:
    /**
     * @brief A method for deserializing JSON into the current object state.
     *
     * @param json The JSON string
     *
     * @return a Status indicating the success or failure of the operation.
     */
    Status fromJSON(const std::string& json);

public:
    std::string id;
    std::string corporate_actions_id;
    std::string ca_type;
    std::string ca_sub_type;
    std::string initiating_symbol;
    std::string initiating_original_cusip;
    std::string target_symbol;
    std::string target_original_cusip;
    std::string declaration_date;
    std::string expiration_date;
    std::string record_date;
    std::string payable_date;
    std::string cash;
    std::string old_rate;
    std::string new_rate;
};

}  // namespace alpaca::markets
