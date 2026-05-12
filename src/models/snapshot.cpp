#include <alpaca/markets/snapshot.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

namespace {

// Helper to dispatch a nested-object sub-key to a target model's fromJSON.
Status parseNested(const glz::generic::object_t& obj, const char* key, auto& target_model) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_object()) {
        return Status();  // optional - absence is fine
    }
    return target_model.fromJSON(json_detail::write(it->second));
}

}  // namespace

Status Snapshot::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("snapshot");

    if (Status s = parseNested(node, "latestTrade", latest_trade); !s.ok()) {
        return s;
    }
    if (Status s = parseNested(node, "latestQuote", latest_quote); !s.ok()) {
        return s;
    }
    if (Status s = parseNested(node, "minuteBar", minute_bar); !s.ok()) {
        return s;
    }
    if (Status s = parseNested(node, "dailyBar", daily_bar); !s.ok()) {
        return s;
    }
    if (Status s = parseNested(node, "prevDailyBar", prev_daily_bar); !s.ok()) {
        return s;
    }

    return Status();
}

Status Snapshots::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("snapshots");

    // Parse snapshots - keyed by symbol
    glz::generic::object_t::const_iterator it = node.find("snapshots");
    if (it != node.end() && it->second.is_object()) {
        for (const auto& [symbol, value] : it->second.get_object()) {
            Snapshot snapshot;
            if (Status status = snapshot.fromJSON(json_detail::write(value)); !status.ok()) {
                return status;
            }
            snapshots[symbol] = snapshot;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
