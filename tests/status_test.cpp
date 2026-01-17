#include <alpaca/markets/status.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(StatusTest, DefaultConstructor) {
    Status status;
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(status.getCode(), 0);
    EXPECT_EQ(status.getMessage(), "OK");
}

TEST(StatusTest, ErrorStatus) {
    Status status(1, "Error message");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.getCode(), 1);
    EXPECT_EQ(status.getMessage(), "Error message");
}

TEST(StatusTest, BoolConversion) {
    Status ok_status;
    Status error_status(1, "Error");
    
    EXPECT_TRUE(static_cast<bool>(ok_status));
    EXPECT_FALSE(static_cast<bool>(error_status));
}

TEST(StatusTest, Equality) {
    Status status1(0, "OK");
    Status status2(0, "OK");
    Status status3(1, "Error");
    
    EXPECT_EQ(status1, status2);
    EXPECT_NE(status1, status3);
}

TEST(StatusTest, ToString) {
    Status status(42, "Test message");
    EXPECT_EQ(status.toString(), "Test message");
    EXPECT_EQ(status.what(), "Test message");
}

TEST(ActionStatusTest, ToStringConversion) {
    EXPECT_EQ(actionStatusToString(ActionStatus::Open), "open");
    EXPECT_EQ(actionStatusToString(ActionStatus::Closed), "closed");
    EXPECT_EQ(actionStatusToString(ActionStatus::Active), "active");
    EXPECT_EQ(actionStatusToString(ActionStatus::All), "all");
}

TEST(APIErrorTest, BasicError) {
    APIError err(422, 40010000, "insufficient qty available for order");
    
    EXPECT_EQ(err.getHTTPStatusCode(), 422);
    EXPECT_EQ(err.getAPICode(), 40010000);
    EXPECT_EQ(err.getMessage(), "insufficient qty available for order");
    EXPECT_EQ(err.what(), "insufficient qty available for order (HTTP 422, Code 40010000)");
}

TEST(APIErrorTest, ErrorWithoutCode) {
    APIError err(403, 0, "forbidden");
    
    EXPECT_EQ(err.getHTTPStatusCode(), 403);
    EXPECT_EQ(err.getAPICode(), 0);
    EXPECT_EQ(err.what(), "forbidden (HTTP 403)");
}

TEST(APIErrorTest, ToStatus) {
    APIError err(400, 40010001, "Bad request");
    Status status = err.toStatus();
    
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.getMessage(), "Bad request (HTTP 400, Code 40010001)");
}
