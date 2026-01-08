#include <alpaca/markets/streaming.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets::stream;

TEST(MessageGeneratorTest, Authentication) {
    MessageGenerator gen;
    std::string msg = gen.authentication("test-key-id", "test-secret-key");
    
    // Verify it's valid JSON containing expected fields
    EXPECT_NE(msg.find("authenticate"), std::string::npos);
    EXPECT_NE(msg.find("test-key-id"), std::string::npos);
    EXPECT_NE(msg.find("test-secret-key"), std::string::npos);
}

TEST(MessageGeneratorTest, Listen) {
    MessageGenerator gen;
    std::set<StreamType> streams = {StreamType::TradeUpdates, StreamType::AccountUpdates};
    std::string msg = gen.listen(streams);
    
    // Verify it's valid JSON containing expected fields
    EXPECT_NE(msg.find("listen"), std::string::npos);
    EXPECT_NE(msg.find("trade_updates"), std::string::npos);
    EXPECT_NE(msg.find("account_updates"), std::string::npos);
}

TEST(ParseReplyTest, Authorization) {
    const std::string json = R"({"stream": "authorization", "data": {}})";
    auto [status, reply] = parseReply(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(reply.reply_type, ReplyType::Authorization);
}

TEST(ParseReplyTest, Listening) {
    const std::string json = R"({"stream": "listening", "data": {}})";
    auto [status, reply] = parseReply(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(reply.reply_type, ReplyType::Listening);
}

TEST(ParseReplyTest, TradeUpdates) {
    const std::string json = R"({"stream": "trade_updates", "data": {"event": "fill"}})";
    auto [status, reply] = parseReply(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(reply.reply_type, ReplyType::Update);
    EXPECT_EQ(reply.stream_type, StreamType::TradeUpdates);
    EXPECT_NE(reply.data.find("fill"), std::string::npos);
}

TEST(ParseReplyTest, AccountUpdates) {
    const std::string json = R"({"stream": "account_updates", "data": {"id": "123"}})";
    auto [status, reply] = parseReply(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(reply.reply_type, ReplyType::Update);
    EXPECT_EQ(reply.stream_type, StreamType::AccountUpdates);
}

TEST(ParseReplyTest, InvalidJSON) {
    auto [status, reply] = parseReply("invalid json");
    EXPECT_FALSE(status.ok());
}

TEST(ParseReplyTest, UnknownStream) {
    const std::string json = R"({"stream": "unknown_stream", "data": {}})";
    auto [status, reply] = parseReply(json);
    EXPECT_FALSE(status.ok());
}
