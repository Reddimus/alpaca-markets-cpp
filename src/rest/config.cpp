#include <alpaca/markets/config.hpp>

#include <cstdlib>
#include <iostream>

namespace alpaca::markets {

namespace {
// Helper to extract hostname from URL
std::string extractHostname(const std::string& url) {
    std::string host = url;
    
    // Remove scheme if present
    if (host.find("https://") == 0) {
        host = host.substr(8);
    } else if (host.find("http://") == 0) {
        host = host.substr(7);
    } else if (host.find("wss://") == 0) {
        host = host.substr(6);
    } else if (host.find("ws://") == 0) {
        host = host.substr(5);
    }
    
    // Remove path if present
    size_t pos = host.find('/');
    if (pos != std::string::npos) {
        host = host.substr(0, pos);
    }
    
    // Remove port if present
    pos = host.find(':');
    if (pos != std::string::npos) {
        host = host.substr(0, pos);
    }
    
    return host;
}

// Helper to ensure URL has scheme
std::string ensureHttpsScheme(const std::string& url) {
    if (url.empty()) return url;
    if (url.find("://") != std::string::npos) {
        return url;  // Already has scheme
    }
    return "https://" + url;
}

// Helper to derive stream URL from base URL
std::string deriveStreamURL(const std::string& base_url) {
    std::string host = extractHostname(base_url);
    return "wss://" + host + "/stream";
}
}  // namespace

Status Environment::parse() {
    const char* error_suffix = " environment variable not set";

    // Try new naming first, fall back to legacy
    if (const char* e = std::getenv("ALPACA_MARKETS_KEY_ID")) {
        api_key_id_ = std::string(e);
    } else if (const char* e = std::getenv(api_key_id_env_var_.c_str())) {
        api_key_id_ = std::string(e);
    } else {
        return Status(1, api_key_id_env_var_ + error_suffix);
    }

    if (const char* e = std::getenv("ALPACA_MARKETS_SECRET_KEY")) {
        api_secret_key_ = std::string(e);
    } else if (const char* e = std::getenv(api_secret_key_env_var_.c_str())) {
        api_secret_key_ = std::string(e);
    } else {
        return Status(1, api_secret_key_env_var_ + error_suffix);
    }

    // Trading base URL (with full scheme)
    if (const char* e = std::getenv("ALPACA_MARKETS_TRADING_URL")) {
        trading_base_url_ = ensureHttpsScheme(std::string(e));
    } else if (const char* e = std::getenv(trading_base_url_env_var_.c_str())) {
        trading_base_url_ = ensureHttpsScheme(std::string(e));
    } else {
        std::cerr << "Warning: " << trading_base_url_env_var_ << " not set, defaulting to paper trading URL\n";
        trading_base_url_ = kTradingBaseURLPaper;
    }

    // Data base URL
    if (const char* e = std::getenv("ALPACA_MARKETS_DATA_URL")) {
        data_base_url_ = ensureHttpsScheme(std::string(e));
    } else if (const char* e = std::getenv(data_base_url_env_var_.c_str())) {
        data_base_url_ = ensureHttpsScheme(std::string(e));
    } else {
        data_base_url_ = kDataBaseURL;
    }

    // Trading stream URL
    if (!trading_stream_url_env_var_.empty()) {
        if (const char* e = std::getenv(trading_stream_url_env_var_.c_str())) {
            trading_stream_url_ = std::string(e);
        }
    }
    if (trading_stream_url_.empty()) {
        if (const char* e = std::getenv("ALPACA_MARKETS_STREAM_URL")) {
            trading_stream_url_ = std::string(e);
        } else {
            // Derive from trading base URL
            trading_stream_url_ = deriveStreamURL(trading_base_url_);
        }
    }

    parsed_ = true;
    return Status();
}

bool Environment::hasBeenParsed() const {
    return parsed_;
}

std::string Environment::getAPIKeyID() const {
    return api_key_id_;
}

std::string Environment::getAPISecretKey() const {
    return api_secret_key_;
}

std::string Environment::getTradingBaseURL() const {
    return trading_base_url_;
}

std::string Environment::getDataBaseURL() const {
    return data_base_url_;
}

std::string Environment::getTradingStreamURL() const {
    return trading_stream_url_;
}

std::string Environment::getTradingHost() const {
    return extractHostname(trading_base_url_);
}

std::string Environment::getDataHost() const {
    return extractHostname(data_base_url_);
}

}  // namespace alpaca::markets
