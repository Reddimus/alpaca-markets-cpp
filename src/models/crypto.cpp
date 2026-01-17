#include <alpaca/markets/crypto.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

std::string cryptoFeedToString(CryptoFeed feed) {
    switch (feed) {
        case CryptoFeed::US:
            return "us";
        case CryptoFeed::Global:
            return "global";
        default:
            return "us";
    }
}

CryptoFeed stringToCryptoFeed(const std::string& s) {
    if (s == "global") {
        return CryptoFeed::Global;
    }
    return CryptoFeed::US;
}

Status CryptoTrade::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto trade JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto trade object");
    }

    PARSE_DOUBLE(price, "p")
    PARSE_UINT64(size, "s")
    PARSE_STRING(timestamp, "t")
    PARSE_UINT64(id, "i")
    PARSE_STRING(taker_side, "tks")

    return Status();
}

Status CryptoQuote::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto quote JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto quote object");
    }

    PARSE_DOUBLE(ask_price, "ap")
    PARSE_DOUBLE(ask_size, "as")
    PARSE_DOUBLE(bid_price, "bp")
    PARSE_DOUBLE(bid_size, "bs")
    PARSE_STRING(timestamp, "t")

    return Status();
}

Status CryptoBar::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto bar JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto bar object");
    }

    PARSE_STRING(timestamp, "t")
    PARSE_DOUBLE(open_price, "o")
    PARSE_DOUBLE(high_price, "h")
    PARSE_DOUBLE(low_price, "l")
    PARSE_DOUBLE(close_price, "c")
    PARSE_DOUBLE(volume, "v")
    PARSE_UINT64(trade_count, "n")
    PARSE_DOUBLE(vwap, "vw")

    return Status();
}

Status CryptoSnapshot::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto snapshot JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto snapshot object");
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

Status CryptoTrades::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto trades JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto trades object");
    }

    if (d.HasMember("trades") && d["trades"].IsObject()) {
        for (auto& m : d["trades"].GetObject()) {
            std::string symbol = m.name.GetString();
            std::vector<CryptoTrade> symbol_trades;

            if (m.value.IsArray()) {
                for (auto& item : m.value.GetArray()) {
                    CryptoTrade trade;
                    rapidjson::StringBuffer s;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    item.Accept(writer);
                    if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_trades.push_back(trade);
                }
            }
            trades[symbol] = symbol_trades;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

Status CryptoQuotes::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto quotes JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto quotes object");
    }

    if (d.HasMember("quotes") && d["quotes"].IsObject()) {
        for (auto& m : d["quotes"].GetObject()) {
            std::string symbol = m.name.GetString();
            std::vector<CryptoQuote> symbol_quotes;

            if (m.value.IsArray()) {
                for (auto& item : m.value.GetArray()) {
                    CryptoQuote quote;
                    rapidjson::StringBuffer s;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    item.Accept(writer);
                    if (Status status = quote.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_quotes.push_back(quote);
                }
            }
            quotes[symbol] = symbol_quotes;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

Status CryptoBars::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing crypto bars JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a crypto bars object");
    }

    if (d.HasMember("bars") && d["bars"].IsObject()) {
        for (auto& m : d["bars"].GetObject()) {
            std::string symbol = m.name.GetString();
            std::vector<CryptoBar> symbol_bars;

            if (m.value.IsArray()) {
                for (auto& item : m.value.GetArray()) {
                    CryptoBar bar;
                    rapidjson::StringBuffer s;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    item.Accept(writer);
                    if (Status status = bar.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_bars.push_back(bar);
                }
            }
            bars[symbol] = symbol_bars;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

}  // namespace alpaca::markets
