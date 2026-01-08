#include <alpaca/markets/streaming.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

namespace alpaca::markets::stream {

namespace {
const std::string kAuthorizationStream = "authorization";
const std::string kListeningStream = "listening";
const std::string kTradeUpdatesStream = "trade_updates";
const std::string kAccountUpdatesStream = "account_updates";

std::string streamToString(StreamType stream) {
    switch (stream) {
        case StreamType::TradeUpdates:
            return "trade_updates";
        case StreamType::AccountUpdates:
            return "account_updates";
        case StreamType::Unknown:
        default:
            return "unknown";
    }
}
}  // namespace

std::string MessageGenerator::authentication(const std::string& key_id, const std::string& secret_key) const {
    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("action");
    writer.String("authenticate");
    writer.Key("data");
    writer.StartObject();
    writer.Key("key_id");
    writer.String(key_id.c_str());
    writer.Key("secret_key");
    writer.String(secret_key.c_str());
    writer.EndObject();
    writer.EndObject();
    return s.GetString();
}

std::string MessageGenerator::listen(const std::set<StreamType>& streams) const {
    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("action");
    writer.String("listen");
    writer.Key("data");
    writer.StartObject();
    writer.Key("streams");
    writer.StartArray();
    for (const auto& stream : streams) {
        writer.String(streamToString(stream).c_str());
    }
    writer.EndArray();
    writer.EndObject();
    writer.EndObject();
    return s.GetString();
}

std::pair<Status, Reply> parseReply(const std::string& text) {
    Reply r;

    rapidjson::Document d;
    if (d.Parse(text.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing reply JSON"), r);
    }

    if (!d.IsObject()) {
        return std::make_pair(Status(1, "Deserialized valid JSON but it wasn't an object"), r);
    }

    if (d.HasMember("stream") && d["stream"].IsString()) {
        std::string stream = d["stream"].GetString();
        if (stream == kAuthorizationStream) {
            r.reply_type = ReplyType::Authorization;
            return std::make_pair(Status(), r);
        } else if (stream == kListeningStream) {
            r.reply_type = ReplyType::Listening;
            return std::make_pair(Status(), r);
        } else if (stream == kTradeUpdatesStream) {
            r.reply_type = ReplyType::Update;
            r.stream_type = StreamType::TradeUpdates;
        } else if (stream == kAccountUpdatesStream) {
            r.reply_type = ReplyType::Update;
            r.stream_type = StreamType::AccountUpdates;
        } else {
            std::ostringstream ss;
            ss << "Unknown stream string: " << stream;
            return std::make_pair(Status(1, ss.str()), r);
        }
    } else {
        return std::make_pair(Status(1, "Reply did not contain stream key"), r);
    }

    if (d.HasMember("data") && d["data"].IsObject()) {
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["data"].Accept(writer);
        r.data = s.GetString();
    }

    return std::make_pair(Status(), r);
}

Status Handler::run(Environment& env) {
    if (!env.hasBeenParsed()) {
        if (auto status = env.parse(); !status.ok()) {
            return status;
        }
    }

    // Note: Full WebSocket streaming requires additional dependencies.
    // This is a placeholder that documents the expected behavior.
    // To implement streaming, integrate with a WebSocket library like:
    // - Boost.Beast
    // - websocketpp
    // - libwebsockets
    
    return Status(1, "Streaming not yet implemented - requires WebSocket library integration");
}

}  // namespace alpaca::markets::stream
