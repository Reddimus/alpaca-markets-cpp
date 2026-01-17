#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief Crypto feed type for selecting data source
 */
enum class CryptoFeed {
    US,      // US feed (default)
    Global,  // Global feed
};

/**
 * @brief A type representing a crypto trade.
 */
class CryptoTrade {
public:
    Status fromJSON(const std::string& json);

public:
    double price = 0.0;           // "p"
    uint64_t size = 0;            // "s"
    std::string timestamp;        // "t" - ISO 8601
    uint64_t id = 0;              // "i" - trade ID
    std::string taker_side;       // "tks" - "B" or "S"
};

/**
 * @brief A type representing a crypto quote.
 */
class CryptoQuote {
public:
    Status fromJSON(const std::string& json);

public:
    double ask_price = 0.0;      // "ap"
    double ask_size = 0.0;       // "as" - can be decimal for crypto
    double bid_price = 0.0;      // "bp"
    double bid_size = 0.0;       // "bs" - can be decimal for crypto
    std::string timestamp;       // "t" - ISO 8601
};

/**
 * @brief A type representing a crypto bar (OHLCV data).
 */
class CryptoBar {
public:
    Status fromJSON(const std::string& json);

public:
    std::string timestamp;        // "t"
    double open_price = 0.0;      // "o"
    double high_price = 0.0;      // "h"
    double low_price = 0.0;       // "l"
    double close_price = 0.0;     // "c"
    double volume = 0.0;          // "v" - can be decimal for crypto
    uint64_t trade_count = 0;     // "n"
    double vwap = 0.0;            // "vw"
};

/**
 * @brief A type representing a crypto snapshot.
 */
class CryptoSnapshot {
public:
    Status fromJSON(const std::string& json);

public:
    CryptoTrade latest_trade;
    CryptoQuote latest_quote;
    CryptoBar minute_bar;
    CryptoBar daily_bar;
    CryptoBar prev_daily_bar;
};

/**
 * @brief Response containing crypto trades with pagination
 */
class CryptoTrades {
public:
    Status fromJSON(const std::string& json);

public:
    std::map<std::string, std::vector<CryptoTrade>> trades;
    std::string next_page_token;
};

/**
 * @brief Response containing crypto quotes with pagination
 */
class CryptoQuotes {
public:
    Status fromJSON(const std::string& json);

public:
    std::map<std::string, std::vector<CryptoQuote>> quotes;
    std::string next_page_token;
};

/**
 * @brief Response containing crypto bars with pagination
 */
class CryptoBars {
public:
    Status fromJSON(const std::string& json);

public:
    std::map<std::string, std::vector<CryptoBar>> bars;
    std::string next_page_token;
};

// Conversion functions
std::string cryptoFeedToString(CryptoFeed feed);
CryptoFeed stringToCryptoFeed(const std::string& s);

}  // namespace alpaca::markets
