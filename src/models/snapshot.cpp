#include <alpaca/markets/snapshot.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status Snapshot::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing snapshot JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a snapshot object");
    }

    // Parse latest trade
    if (d.HasMember("latestTrade") && d["latestTrade"].IsObject()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["latestTrade"].Accept(writer);
        if (Status status = latest_trade.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    // Parse latest quote
    if (d.HasMember("latestQuote") && d["latestQuote"].IsObject()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["latestQuote"].Accept(writer);
        if (Status status = latest_quote.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    // Parse minute bar
    if (d.HasMember("minuteBar") && d["minuteBar"].IsObject()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["minuteBar"].Accept(writer);
        if (Status status = minute_bar.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    // Parse daily bar
    if (d.HasMember("dailyBar") && d["dailyBar"].IsObject()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["dailyBar"].Accept(writer);
        if (Status status = daily_bar.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    // Parse previous daily bar
    if (d.HasMember("prevDailyBar") && d["prevDailyBar"].IsObject()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["prevDailyBar"].Accept(writer);
        if (Status status = prev_daily_bar.fromJSON(s.GetString()); !status.ok()) {
            return status;
        }
    }

    return Status();
}

Status Snapshots::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing snapshots JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a snapshots object");
    }

    // Parse snapshots - keyed by symbol
    if (d.HasMember("snapshots") && d["snapshots"].IsObject()) {
        for (auto& m : d["snapshots"].GetObject()) {
            std::string symbol = m.name.GetString();
            Snapshot snapshot;
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = snapshot.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            snapshots[symbol] = snapshot;
        }
    }

    return Status();
}

}  // namespace alpaca::markets
