#include <alpaca/markets/crypto.hpp>

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
    JSON_FROMJSON_PRELUDE("crypto trade");

    PARSE_DOUBLE(price, "p");
    PARSE_UINT64(size, "s");
    PARSE_STRING(timestamp, "t");
    PARSE_UINT64(id, "i");
    PARSE_STRING(taker_side, "tks");

    return Status();
}

Status CryptoQuote::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto quote");

    PARSE_DOUBLE(ask_price, "ap");
    PARSE_DOUBLE(ask_size, "as");
    PARSE_DOUBLE(bid_price, "bp");
    PARSE_DOUBLE(bid_size, "bs");
    PARSE_STRING(timestamp, "t");

    return Status();
}

Status CryptoBar::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto bar");

    PARSE_STRING(timestamp, "t");
    PARSE_DOUBLE(open_price, "o");
    PARSE_DOUBLE(high_price, "h");
    PARSE_DOUBLE(low_price, "l");
    PARSE_DOUBLE(close_price, "c");
    PARSE_DOUBLE(volume, "v");
    PARSE_UINT64(trade_count, "n");
    PARSE_DOUBLE(vwap, "vw");

    return Status();
}

namespace {

Status parseNested(const glz::generic::object_t& obj, const char* key, auto& target_model) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_object()) {
        return Status();
    }
    return target_model.fromJSON(json_detail::write(it->second));
}

}  // namespace

Status CryptoSnapshot::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto snapshot");

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

Status CryptoTrades::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto trades");

    glz::generic::object_t::const_iterator trades_it = node.find("trades");
    if (trades_it != node.end() && trades_it->second.is_object()) {
        for (const auto& [symbol, value] : trades_it->second.get_object()) {
            std::vector<CryptoTrade> symbol_trades;
            if (value.is_array()) {
                for (const glz::generic& item : value.get_array()) {
                    CryptoTrade trade;
                    if (Status status = trade.fromJSON(json_detail::write(item)); !status.ok()) {
                        return status;
                    }
                    symbol_trades.push_back(trade);
                }
            }
            trades[symbol] = symbol_trades;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

Status CryptoQuotes::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto quotes");

    glz::generic::object_t::const_iterator quotes_it = node.find("quotes");
    if (quotes_it != node.end() && quotes_it->second.is_object()) {
        for (const auto& [symbol, value] : quotes_it->second.get_object()) {
            std::vector<CryptoQuote> symbol_quotes;
            if (value.is_array()) {
                for (const glz::generic& item : value.get_array()) {
                    CryptoQuote quote;
                    if (Status status = quote.fromJSON(json_detail::write(item)); !status.ok()) {
                        return status;
                    }
                    symbol_quotes.push_back(quote);
                }
            }
            quotes[symbol] = symbol_quotes;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

Status CryptoBars::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("crypto bars");

    glz::generic::object_t::const_iterator bars_it = node.find("bars");
    if (bars_it != node.end() && bars_it->second.is_object()) {
        for (const auto& [symbol, value] : bars_it->second.get_object()) {
            std::vector<CryptoBar> symbol_bars;
            if (value.is_array()) {
                for (const glz::generic& item : value.get_array()) {
                    CryptoBar bar;
                    if (Status status = bar.fromJSON(json_detail::write(item)); !status.ok()) {
                        return status;
                    }
                    symbol_bars.push_back(bar);
                }
            }
            bars[symbol] = symbol_bars;
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
