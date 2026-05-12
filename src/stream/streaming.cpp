#include <alpaca/markets/streaming.hpp>

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

#include <sstream>
#include <string>
#include <string_view>

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
    // Build the {action, data: {key_id, secret_key}} envelope as a typed
    // glz::generic so we don't have to declare a one-off struct. Glaze writes
    // ordered_map keys in insertion order, matching the pre-migration
    // RapidJSON Writer output.
    glz::generic root = glz::generic::object_t{};
    glz::generic::object_t& obj = root.get_object();
    obj["action"] = std::string("authenticate");

    glz::generic data = glz::generic::object_t{};
    glz::generic::object_t& data_obj = data.get_object();
    data_obj["key_id"] = key_id;
    data_obj["secret_key"] = secret_key;
    obj["data"] = std::move(data);

    std::string out;
    (void)glz::write_json(root, out);
    return out;
}

std::string MessageGenerator::listen(const std::set<StreamType>& streams) const {
    glz::generic root = glz::generic::object_t{};
    glz::generic::object_t& obj = root.get_object();
    obj["action"] = std::string("listen");

    glz::generic data = glz::generic::object_t{};
    glz::generic::object_t& data_obj = data.get_object();

    glz::generic streams_arr = glz::generic::array_t{};
    glz::generic::array_t& arr = streams_arr.get_array();
    for (const StreamType& stream : streams) {
        arr.emplace_back(streamToString(stream));
    }
    data_obj["streams"] = std::move(streams_arr);
    obj["data"] = std::move(data);

    std::string out;
    (void)glz::write_json(root, out);
    return out;
}

std::pair<Status, Reply> parseReply(const std::string& text) {
    Reply r;

    // Stream replies are a typed envelope:
    //   {"stream": "<name>", "data": <opaque object>}
    //
    // The "data" payload's schema depends on the stream type. We keep
    // the existing contract: pass the data sub-object through to the
    // caller as a JSON string so per-stream handlers can re-parse with
    // their own schema. The outer envelope is parsed via glz::generic
    // because Glaze's tagged-union support (glz::tag, glz::ids) maps a
    // stream-name discriminator to *typed* alternatives, but our
    // sub-models for trade_updates / account_updates are not declared
    // here (they live in downstream consumers' code).
    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, text); ec) {
        return {Status(1, "Received parse error when deserializing reply JSON"), r};
    }
    if (!root.is_object()) {
        return {Status(1, "Deserialized valid JSON but it wasn't an object"), r};
    }
    const glz::generic::object_t& obj = root.get_object();

    glz::generic::object_t::const_iterator stream_it = obj.find("stream");
    if (stream_it == obj.end() || !stream_it->second.is_string()) {
        return {Status(1, "Reply did not contain stream key"), r};
    }
    const std::string stream = stream_it->second.get<std::string>();
    if (stream == kAuthorizationStream) {
        r.reply_type = ReplyType::Authorization;
        return {Status(), r};
    } else if (stream == kListeningStream) {
        r.reply_type = ReplyType::Listening;
        return {Status(), r};
    } else if (stream == kTradeUpdatesStream) {
        r.reply_type = ReplyType::Update;
        r.stream_type = StreamType::TradeUpdates;
    } else if (stream == kAccountUpdatesStream) {
        r.reply_type = ReplyType::Update;
        r.stream_type = StreamType::AccountUpdates;
    } else {
        std::ostringstream ss;
        ss << "Unknown stream string: " << stream;
        return {Status(1, ss.str()), r};
    }

    glz::generic::object_t::const_iterator data_it = obj.find("data");
    if (data_it != obj.end() && data_it->second.is_object()) {
        std::string data_json;
        (void)glz::write_json(data_it->second, data_json);
        r.data = std::move(data_json);
    }

    return {Status(), r};
}

Status Handler::run(Environment& env) {
    if (!env.hasBeenParsed()) {
        if (Status status = env.parse(); !status.ok()) {
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
