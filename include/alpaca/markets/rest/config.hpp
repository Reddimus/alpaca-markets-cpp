#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>

namespace alpaca::markets {

/// The base URL for API calls to the live trading API
inline const std::string kTradingBaseURLLive = "https://api.alpaca.markets";

/// The base URL for API calls to the paper trading API
inline const std::string kTradingBaseURLPaper = "https://paper-api.alpaca.markets";

/// The base URL for API calls to the data API
inline const std::string kDataBaseURL = "https://data.alpaca.markets";

/// The WebSocket URL for trading stream (live)
inline const std::string kTradingStreamURLLive = "wss://api.alpaca.markets/stream";

/// The WebSocket URL for trading stream (paper)
inline const std::string kTradingStreamURLPaper = "wss://paper-api.alpaca.markets/stream";

/**
 * @brief A class to help with parsing required variables from the environment.
 *
 * The Environment class supports both the legacy APCA_* environment variables
 * and the newer ALPACA_MARKETS_* variables for compatibility.
 *
 * @code{.cpp}
 *   auto env = alpaca::markets::Environment();
 *   if (auto status = env.parse(); !status.ok()) {
 *     std::cerr << "Error parsing config from environment: "
 *               << status.getMessage() << std::endl;
 *     return status.getCode();
 *   }
 * @endcode
 */
class Environment {
public:
    /**
     * @brief Default constructor
     *
     * Uses default environment variable names. Supports both legacy APCA_* 
     * and newer ALPACA_MARKETS_* naming conventions.
     */
    Environment() = default;

    /**
     * @brief Constructor with custom environment variable names
     *
     * Use this constructor if you'd like to parse the required environment 
     * variables from non-standard names.
     */
    explicit Environment(std::string api_key_id_env_var, std::string api_secret_key_env_var,
                         std::string trading_base_url_env_var = "", std::string data_base_url_env_var = "",
                         std::string trading_stream_url_env_var = "")
        : api_key_id_env_var_(std::move(api_key_id_env_var)),
          api_secret_key_env_var_(std::move(api_secret_key_env_var)),
          trading_base_url_env_var_(std::move(trading_base_url_env_var)),
          data_base_url_env_var_(std::move(data_base_url_env_var)),
          trading_stream_url_env_var_(std::move(trading_stream_url_env_var)) {}

    /**
     * @brief Parse the environment variables into local state.
     *
     * @return a Status indicating the success or failure of the operation.
     */
    Status parse();

    /**
     * @brief Indicates whether or not the environment has been successfully parsed.
     */
    [[nodiscard]] bool hasBeenParsed() const;

    /**
     * @brief A getter for the API Key ID
     *
     * Note that this method should only be called after successfully calling
     * the parse() method.
     */
    [[nodiscard]] std::string getAPIKeyID() const;

    /**
     * @brief A getter for the API Secret Key
     *
     * Note that this method should only be called after successfully calling
     * the parse() method.
     */
    [[nodiscard]] std::string getAPISecretKey() const;

    /**
     * @brief A getter for the Trading Base URL (REST)
     *
     * Returns full URL with scheme (e.g., "https://paper-api.alpaca.markets")
     * Note that this method should only be called after successfully calling
     * the parse() method.
     */
    [[nodiscard]] std::string getTradingBaseURL() const;

    /**
     * @brief A getter for the Data Base URL (REST)
     *
     * Returns full URL with scheme (e.g., "https://data.alpaca.markets")
     * Note that this method should only be called after successfully calling
     * the parse() method.
     */
    [[nodiscard]] std::string getDataBaseURL() const;

    /**
     * @brief A getter for the Trading Stream URL (WebSocket)
     *
     * Returns full URL with scheme (e.g., "wss://paper-api.alpaca.markets/stream")
     * Note that this method should only be called after successfully calling
     * the parse() method.
     */
    [[nodiscard]] std::string getTradingStreamURL() const;

    /**
     * @brief Get just the hostname from the Trading Base URL (for SSL client)
     */
    [[nodiscard]] std::string getTradingHost() const;

    /**
     * @brief Get just the hostname from the Data Base URL (for SSL client)
     */
    [[nodiscard]] std::string getDataHost() const;

private:
    bool parsed_ = false;

    std::string api_key_id_;
    std::string api_secret_key_;
    std::string trading_base_url_;
    std::string data_base_url_;
    std::string trading_stream_url_;

    // Environment variable names (support both legacy and new naming)
    std::string api_key_id_env_var_ = "APCA_API_KEY_ID";
    std::string api_secret_key_env_var_ = "APCA_API_SECRET_KEY";
    std::string trading_base_url_env_var_ = "APCA_API_BASE_URL";
    std::string data_base_url_env_var_ = "APCA_API_DATA_URL";
    std::string trading_stream_url_env_var_ = "ALPACA_MARKETS_STREAM_URL";
};

}  // namespace alpaca::markets
