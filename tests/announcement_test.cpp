#include <alpaca/markets/announcement.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(AnnouncementTest, FromJSON) {
    const std::string json = R"({
        "id": "ann-12345",
        "corporate_actions_id": "ca-67890",
        "ca_type": "dividend",
        "ca_sub_type": "cash",
        "initiating_symbol": "AAPL",
        "initiating_original_cusip": "037833100",
        "target_symbol": "AAPL",
        "target_original_cusip": "037833100",
        "declaration_date": "2024-01-15",
        "record_date": "2024-02-01",
        "payable_date": "2024-02-15",
        "cash": "0.24",
        "old_rate": "1",
        "new_rate": "1"
    })";

    Announcement announcement;
    Status status = announcement.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(announcement.id, "ann-12345");
    EXPECT_EQ(announcement.corporate_actions_id, "ca-67890");
    EXPECT_EQ(announcement.ca_type, "dividend");
    EXPECT_EQ(announcement.ca_sub_type, "cash");
    EXPECT_EQ(announcement.initiating_symbol, "AAPL");
    EXPECT_EQ(announcement.cash, "0.24");
    EXPECT_EQ(announcement.declaration_date, "2024-01-15");
    EXPECT_EQ(announcement.payable_date, "2024-02-15");
}

TEST(AnnouncementTest, FromJSONParseError) {
    Announcement announcement;
    Status status = announcement.fromJSON("invalid json");
    
    EXPECT_FALSE(status.ok());
}

TEST(AnnouncementTest, SplitAnnouncement) {
    const std::string json = R"({
        "id": "split-123",
        "ca_type": "split",
        "ca_sub_type": "forward",
        "initiating_symbol": "TSLA",
        "old_rate": "1",
        "new_rate": "3"
    })";

    Announcement announcement;
    Status status = announcement.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(announcement.ca_type, "split");
    EXPECT_EQ(announcement.ca_sub_type, "forward");
    EXPECT_EQ(announcement.old_rate, "1");
    EXPECT_EQ(announcement.new_rate, "3");
}

TEST(AnnouncementTypeTest, ToStringConversion) {
    EXPECT_EQ(announcementTypeToString(AnnouncementType::Dividend), "dividend");
    EXPECT_EQ(announcementTypeToString(AnnouncementType::Merger), "merger");
    EXPECT_EQ(announcementTypeToString(AnnouncementType::Spinoff), "spinoff");
    EXPECT_EQ(announcementTypeToString(AnnouncementType::Split), "split");
}

TEST(AnnouncementDateTypeTest, ToStringConversion) {
    EXPECT_EQ(announcementDateTypeToString(AnnouncementDateType::DeclarationDate), "declaration_date");
    EXPECT_EQ(announcementDateTypeToString(AnnouncementDateType::RecordDate), "record_date");
    EXPECT_EQ(announcementDateTypeToString(AnnouncementDateType::ExDate), "ex_date");
    EXPECT_EQ(announcementDateTypeToString(AnnouncementDateType::PayableDate), "payable_date");
}
