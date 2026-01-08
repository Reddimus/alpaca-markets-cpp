#pragma once

#include <alpaca/markets/rest/config.hpp>
#include <alpaca/markets/models/status.hpp>

#include <functional>
#include <set>
#include <string>
#include <utility>

namespace alpaca::markets::stream {

using DataType = std::string;
inline const DataType kDefaultData = "{}";

/**
 * @brief A type representing streams that may be subscribed to
 */
enum class StreamType {
    Unknown,
    TradeUpdates,
    AccountUpdates,
};

/**
 * @brief A type representing the different replies one might receive
 */
enum class ReplyType {
    Unknown,
    Authorization,
    Listening,
    Update,
};

/**
 * @brief A helper class for generating messages for the stream API
 */
class MessageGenerator {
public:
    /**
     * @brief Create an authentication message for the Alpaca stream API
     */
    std::string authentication(const std::string& key_id, const std::string& secret_key) const;

    /**
     * @brief Create message for which stream to listen to for the Alpaca stream API
     */
    std::string listen(const std::set<StreamType>& streams) const;
};

/**
 * @brief A class for handling stream messages
 */
class Handler {
public:
    Handler() = delete;
    Handler(std::function<void(DataType)> on_trade_update, std::function<void(DataType)> on_account_update)
        : on_trade_update_(std::move(on_trade_update)), on_account_update_(std::move(on_account_update)) {}

public:
    /**
     * @brief Run the stream handler and block.
     * 
     * Note: This is a placeholder for streaming support. Full WebSocket 
     * implementation requires additional dependencies (e.g., Boost.Beast).
     */
    Status run(Environment& env);

private:
    std::function<void(DataType)> on_trade_update_;
    std::function<void(DataType)> on_account_update_;
};

/**
 * @brief A class representing a parsed stream reply
 */
class Reply {
public:
    Reply(ReplyType reply_type = ReplyType::Unknown, StreamType stream_type = StreamType::Unknown,
          DataType data = kDefaultData)
        : reply_type(reply_type), stream_type(stream_type), data(std::move(data)) {}

public:
    ReplyType reply_type;
    StreamType stream_type;
    std::string data;
};

/**
 * @brief Parse text from an Alpaca stream into a Reply object
 */
std::pair<Status, Reply> parseReply(const std::string& text);

}  // namespace alpaca::markets::stream
