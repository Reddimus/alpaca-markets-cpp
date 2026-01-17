#pragma once

#include <alpaca/markets/models/account.hpp>
#include <alpaca/markets/models/announcement.hpp>
#include <alpaca/markets/models/asset.hpp>
#include <alpaca/markets/models/auction.hpp>
#include <alpaca/markets/models/bars.hpp>
#include <alpaca/markets/models/calendar.hpp>
#include <alpaca/markets/models/clock.hpp>
#include <alpaca/markets/models/corporate_action.hpp>
#include <alpaca/markets/models/crypto.hpp>
#include <alpaca/markets/models/multi_quote.hpp>
#include <alpaca/markets/models/multi_trade.hpp>
#include <alpaca/markets/models/news.hpp>
#include <alpaca/markets/models/option.hpp>
#include <alpaca/markets/models/order.hpp>
#include <alpaca/markets/models/portfolio.hpp>
#include <alpaca/markets/models/position.hpp>
#include <alpaca/markets/models/quote.hpp>
#include <alpaca/markets/models/snapshot.hpp>
#include <alpaca/markets/models/status.hpp>
#include <alpaca/markets/models/trade.hpp>
#include <alpaca/markets/models/watchlist.hpp>
#include <alpaca/markets/rest/config.hpp>

#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace alpaca::markets {

/**
 * @brief The API client object for interacting with the Alpaca Trading API.
 *
 * @code{.cpp}
 *   alpaca::markets::Environment env;
 *   if (alpaca::markets::Status status = env.parse(); !status.ok()) {
 *     std::cerr << "Error parsing config from environment: "
 *               << status.getMessage() << std::endl;
 *     return status.getCode();
 *   }
 *   alpaca::markets::Client client(env);
 * @endcode
 */
class Client {
public:
    /**
     * @brief The primary constructor.
     */
    explicit Client(Environment& environment);

    /**
     * @brief The default constructor of Client should never be used.
     */
    Client() = delete;

    // ==================== Account ====================

    /**
     * @brief Fetch Alpaca account information.
     */
    std::pair<Status, Account> getAccount() const;

    /**
     * @brief Fetch Alpaca account configuration information.
     */
    std::pair<Status, AccountConfigurations> getAccountConfigurations() const;

    /**
     * @brief Fetch Alpaca account activity.
     */
    std::pair<Status, std::vector<std::variant<TradeActivity, NonTradeActivity>>> getAccountActivity(
        const std::vector<std::string>& activity_types = {}) const;

    /**
     * @brief Update Alpaca account configuration information.
     */
    std::pair<Status, AccountConfigurations> updateAccountConfigurations(bool no_shorting,
                                                                         const std::string& dtbp_check,
                                                                         const std::string& trade_confirm_email,
                                                                         bool suspend_trade) const;

    // ==================== Orders ====================

    /**
     * @brief Fetch submitted Alpaca orders.
     */
    std::pair<Status, std::vector<Order>> getOrders(ActionStatus status = ActionStatus::Open, int limit = 50,
                                                    const std::string& after = "", const std::string& until = "",
                                                    OrderDirection direction = OrderDirection::Descending,
                                                    bool nested = false) const;

    /**
     * @brief Fetch a specific Alpaca order.
     */
    std::pair<Status, Order> getOrder(const std::string& id, bool nested = false) const;

    /**
     * @brief Fetch a specific Alpaca order by client order ID.
     */
    std::pair<Status, Order> getOrderByClientOrderID(const std::string& client_order_id) const;

    /**
     * @brief Submit an Alpaca order.
     */
    std::pair<Status, Order> submitOrder(const std::string& symbol, int quantity, OrderSide side, OrderType type,
                                         OrderTimeInForce tif, const std::string& limit_price = "",
                                         const std::string& stop_price = "", bool extended_hours = false,
                                         const std::string& client_order_id = "",
                                         OrderClass order_class = OrderClass::Simple,
                                         TakeProfitParams* take_profit_params = nullptr,
                                         StopLossParams* stop_loss_params = nullptr,
                                         const std::string& trail_price = "",
                                         const std::string& trail_percent = "") const;

    /**
     * @brief Submit a notional (dollar-amount) order.
     * 
     * Allows placing orders by specifying a dollar amount instead of share quantity.
     * Only valid for market and limit order types.
     */
    std::pair<Status, Order> submitNotionalOrder(const std::string& symbol, const std::string& notional,
                                                  OrderSide side, OrderType type, OrderTimeInForce tif,
                                                  const std::string& limit_price = "",
                                                  bool extended_hours = false,
                                                  const std::string& client_order_id = "") const;

    /**
     * @brief Replace an Alpaca order.
     */
    std::pair<Status, Order> replaceOrder(const std::string& id, int quantity, OrderTimeInForce tif,
                                          const std::string& limit_price = "", const std::string& stop_price = "",
                                          const std::string& client_order_id = "") const;

    /**
     * @brief Cancel all Alpaca orders.
     */
    std::pair<Status, std::vector<Order>> cancelOrders() const;

    /**
     * @brief Cancel a specific Alpaca order.
     */
    std::pair<Status, Order> cancelOrder(const std::string& id) const;

    // ==================== Positions ====================

    /**
     * @brief Fetch all open Alpaca positions.
     */
    std::pair<Status, std::vector<Position>> getPositions() const;

    /**
     * @brief Fetch a position for a given symbol.
     */
    std::pair<Status, Position> getPosition(const std::string& symbol) const;

    /**
     * @brief Close (liquidate) all Alpaca positions.
     */
    std::pair<Status, std::vector<Position>> closePositions() const;

    /**
     * @brief Close (liquidate) the position for a given symbol.
     */
    std::pair<Status, Position> closePosition(const std::string& symbol) const;

    // ==================== Assets ====================

    /**
     * @brief Fetch all open Alpaca assets.
     */
    std::pair<Status, std::vector<Asset>> getAssets(ActionStatus asset_status = ActionStatus::Active,
                                                    AssetClass asset_class = AssetClass::USEquity) const;

    /**
     * @brief Fetch an asset for a given symbol.
     */
    std::pair<Status, Asset> getAsset(const std::string& symbol) const;

    // ==================== Clock & Calendar ====================

    /**
     * @brief Fetch the market clock.
     */
    std::pair<Status, Clock> getClock() const;

    /**
     * @brief Fetch calendar data.
     */
    std::pair<Status, std::vector<Date>> getCalendar(const std::string& start, const std::string& end) const;

    // ==================== Watchlists ====================

    /**
     * @brief Fetch watchlists.
     */
    std::pair<Status, std::vector<Watchlist>> getWatchlists() const;

    /**
     * @brief Fetch a watchlist by ID.
     */
    std::pair<Status, Watchlist> getWatchlist(const std::string& id) const;

    /**
     * @brief Create a watchlist.
     */
    std::pair<Status, Watchlist> createWatchlist(const std::string& name, const std::vector<std::string>& symbols) const;

    /**
     * @brief Update a watchlist.
     */
    std::pair<Status, Watchlist> updateWatchlist(const std::string& id, const std::string& name,
                                                 const std::vector<std::string>& symbols) const;

    /**
     * @brief Delete a watchlist.
     */
    Status deleteWatchlist(const std::string& id) const;

    /**
     * @brief Add an asset to a watchlist.
     */
    std::pair<Status, Watchlist> addSymbolToWatchlist(const std::string& id, const std::string& symbol) const;

    /**
     * @brief Remove an asset from a watchlist.
     */
    std::pair<Status, Watchlist> removeSymbolFromWatchlist(const std::string& id, const std::string& symbol) const;

    // ==================== Portfolio ====================

    /**
     * @brief Fetch portfolio history data.
     */
    std::pair<Status, PortfolioHistory> getPortfolioHistory(const std::string& period = "",
                                                            const std::string& timeframe = "",
                                                            const std::string& date_end = "",
                                                            bool extended_hours = false) const;

    // ==================== Market Data (v2) ====================

    /**
     * @brief Fetch historical bar data.
     * 
     * Uses Market Data API v2.
     */
    std::pair<Status, Bars> getBars(const std::vector<std::string>& symbols, const std::string& start,
                                    const std::string& end, const std::string& timeframe = "1Day",
                                    unsigned int limit = 1000, const std::string& page_token = "") const;

    /**
     * @brief Fetch latest trade details for a symbol.
     * 
     * Uses Market Data API v2.
     */
    std::pair<Status, LatestTrade> getLatestTrade(const std::string& symbol) const;

    /**
     * @brief Fetch latest quote details for a symbol.
     * 
     * Uses Market Data API v2.
     */
    std::pair<Status, LatestQuote> getLatestQuote(const std::string& symbol) const;

    /**
     * @brief Fetch latest trades for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @return Map of symbol to LatestTrade
     */
    std::pair<Status, std::map<std::string, Trade>> getLatestTrades(const std::vector<std::string>& symbols) const;

    /**
     * @brief Fetch latest quotes for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @return Map of symbol to Quote
     */
    std::pair<Status, std::map<std::string, Quote>> getLatestQuotes(const std::vector<std::string>& symbols) const;

    // ==================== Corporate Actions ====================

    /**
     * @brief Fetch corporate action announcements.
     * 
     * @param ca_types List of announcement types to filter by (dividend, merger, spinoff, split)
     * @param since Start date for the query (YYYY-MM-DD)
     * @param until End date for the query (YYYY-MM-DD)
     * @param symbol Filter by symbol
     * @param cusip Filter by CUSIP
     * @param date_type Type of date to filter by (declaration_date, record_date, ex_date, payable_date)
     */
    std::pair<Status, std::vector<Announcement>> getAnnouncements(
        const std::vector<std::string>& ca_types = {},
        const std::string& since = "",
        const std::string& until = "",
        const std::string& symbol = "",
        const std::string& cusip = "",
        const std::string& date_type = "") const;

    /**
     * @brief Fetch a specific corporate action announcement by ID.
     */
    std::pair<Status, Announcement> getAnnouncement(const std::string& id) const;

    // ==================== Options ====================

    /**
     * @brief Fetch option contracts.
     * 
     * @param underlying_symbols Comma-separated underlying symbols to filter by
     * @param status Filter by status (active, inactive)
     * @param expiration_date Filter by exact expiration date (YYYY-MM-DD)
     * @param expiration_date_gte Filter by minimum expiration date (YYYY-MM-DD)
     * @param expiration_date_lte Filter by maximum expiration date (YYYY-MM-DD)
     * @param root_symbol Filter by root symbol
     * @param type Filter by option type (call, put)
     * @param style Filter by option style (american, european)
     * @param strike_price_gte Filter by minimum strike price
     * @param strike_price_lte Filter by maximum strike price
     * @param limit Maximum number of results (default 100, max 10000)
     * @param page_token Pagination token
     */
    std::pair<Status, OptionContracts> getOptionContracts(
        const std::string& underlying_symbols = "",
        const std::string& status = "",
        const std::string& expiration_date = "",
        const std::string& expiration_date_gte = "",
        const std::string& expiration_date_lte = "",
        const std::string& root_symbol = "",
        const std::string& type = "",
        const std::string& style = "",
        const std::string& strike_price_gte = "",
        const std::string& strike_price_lte = "",
        unsigned int limit = 100,
        const std::string& page_token = "") const;

    /**
     * @brief Fetch a specific option contract by symbol or ID.
     */
    std::pair<Status, OptionContract> getOptionContract(const std::string& symbol_or_id) const;

    // ==================== Market Data - Snapshots ====================

    /**
     * @brief Fetch market snapshot for a symbol.
     * 
     * Contains latest trade, quote, minute bar, daily bar, and previous daily bar.
     * Uses Market Data API v2.
     */
    std::pair<Status, Snapshot> getSnapshot(const std::string& symbol) const;

    /**
     * @brief Fetch market snapshots for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @return Map of symbol to Snapshot
     */
    std::pair<Status, std::map<std::string, Snapshot>> getSnapshots(const std::vector<std::string>& symbols) const;

    // ==================== Market Data - Latest Bars ====================

    /**
     * @brief Fetch latest bar for a symbol.
     * 
     * Uses Market Data API v2.
     */
    std::pair<Status, Bar> getLatestBar(const std::string& symbol) const;

    /**
     * @brief Fetch latest bars for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @return Map of symbol to Bar
     */
    std::pair<Status, std::map<std::string, Bar>> getLatestBars(const std::vector<std::string>& symbols) const;

    // ==================== Market Data - Historical Trades/Quotes ====================

    /**
     * @brief Fetch historical trades for a symbol.
     * 
     * Uses Market Data API v2.
     * @param symbol Stock symbol
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param limit Maximum number of trades (default 1000, max 10000)
     * @param page_token Pagination token
     * @return Vector of trades and next_page_token for pagination
     */
    std::pair<Status, std::pair<std::vector<Trade>, std::string>> getTrades(
        const std::string& symbol,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    /**
     * @brief Fetch historical quotes for a symbol.
     * 
     * Uses Market Data API v2.
     * @param symbol Stock symbol
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param limit Maximum number of quotes (default 1000, max 10000)
     * @param page_token Pagination token
     * @return Vector of quotes and next_page_token for pagination
     */
    std::pair<Status, std::pair<std::vector<Quote>, std::string>> getQuotes(
        const std::string& symbol,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    // ==================== Market Data - Multi-Symbol Historical ====================

    /**
     * @brief Fetch historical trades for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @param symbols Stock symbols
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param limit Maximum number of trades per symbol (default 1000, max 10000)
     * @param page_token Pagination token
     * @return MultiTrades containing trades per symbol and next_page_token
     */
    std::pair<Status, MultiTrades> getMultiTrades(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    /**
     * @brief Fetch historical quotes for multiple symbols.
     * 
     * Uses Market Data API v2.
     * @param symbols Stock symbols
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param limit Maximum number of quotes per symbol (default 1000, max 10000)
     * @param page_token Pagination token
     * @return MultiQuotes containing quotes per symbol and next_page_token
     */
    std::pair<Status, MultiQuotes> getMultiQuotes(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    // ==================== Market Data - Auctions ====================

    /**
     * @brief Fetch auction data for a single symbol.
     * 
     * Gets opening and closing auction data for a stock.
     * Uses Market Data API v2.
     * @param symbol Stock symbol
     * @param start Start date (YYYY-MM-DD or RFC3339)
     * @param end End date (YYYY-MM-DD or RFC3339)
     * @param limit Maximum number of auctions (default 1000, max 10000)
     * @param page_token Pagination token
     */
    std::pair<Status, Auctions> getAuctions(
        const std::string& symbol,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    /**
     * @brief Fetch auction data for multiple symbols.
     * 
     * Gets opening and closing auction data for multiple stocks.
     * Uses Market Data API v2.
     * @param symbols Stock symbols
     * @param start Start date (YYYY-MM-DD or RFC3339)
     * @param end End date (YYYY-MM-DD or RFC3339)
     * @param limit Maximum number of auctions per symbol (default 1000, max 10000)
     * @param page_token Pagination token
     */
    std::pair<Status, Auctions> getMultiAuctions(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    // ==================== Market Data - Corporate Actions ====================

    /**
     * @brief Fetch corporate actions from Market Data API.
     * 
     * This is distinct from Trading API announcements - it represents market data
     * corporate actions like symbol changes, stock dividends, cash dividends, splits, etc.
     * 
     * Uses Market Data API v1beta1.
     * @param symbols Filter by symbols (comma-separated or vector)
     * @param types Filter by corporate action types (reverse_split, forward_split, unit_split,
     *              cash_dividend, stock_dividend, spin_off, cash_merger, stock_merger,
     *              stock_and_cash_merger, redemption, name_change, worthless_removal)
     * @param start Start date (YYYY-MM-DD)
     * @param end End date (YYYY-MM-DD)
     * @param limit Maximum number of results (default 1000)
     * @param page_token Pagination token
     */
    std::pair<Status, CorporateActions> getCorporateActions(
        const std::vector<std::string>& symbols = {},
        const std::vector<std::string>& types = {},
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "") const;

    // ==================== News API ====================

    /**
     * @brief Fetch news articles.
     * 
     * Uses Market Data API v1beta1.
     * @param symbols Filter by symbols (comma-separated or vector)
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param limit Maximum number of articles (default 50, max 50)
     * @param page_token Pagination token
     * @param include_content Whether to include article content
     * @param exclude_contentless Whether to exclude articles without content
     */
    std::pair<Status, NewsArticles> getNews(
        const std::vector<std::string>& symbols = {},
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 50,
        const std::string& page_token = "",
        bool include_content = false,
        bool exclude_contentless = false) const;

    // ==================== Crypto Market Data ====================

    /**
     * @brief Fetch latest crypto trade for a symbol.
     * 
     * Uses Crypto Market Data API v1beta3.
     * @param symbol Crypto symbol (e.g., "BTC/USD")
     * @param feed Crypto feed (US or Global)
     */
    std::pair<Status, CryptoTrade> getLatestCryptoTrade(
        const std::string& symbol,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch latest crypto trades for multiple symbols.
     * 
     * @param symbols Crypto symbols
     * @param feed Crypto feed
     * @return Map of symbol to CryptoTrade
     */
    std::pair<Status, std::map<std::string, CryptoTrade>> getLatestCryptoTrades(
        const std::vector<std::string>& symbols,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch latest crypto quote for a symbol.
     */
    std::pair<Status, CryptoQuote> getLatestCryptoQuote(
        const std::string& symbol,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch latest crypto quotes for multiple symbols.
     */
    std::pair<Status, std::map<std::string, CryptoQuote>> getLatestCryptoQuotes(
        const std::vector<std::string>& symbols,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch latest crypto bar for a symbol.
     */
    std::pair<Status, CryptoBar> getLatestCryptoBar(
        const std::string& symbol,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch latest crypto bars for multiple symbols.
     */
    std::pair<Status, std::map<std::string, CryptoBar>> getLatestCryptoBars(
        const std::vector<std::string>& symbols,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch crypto snapshot for a symbol.
     */
    std::pair<Status, CryptoSnapshot> getCryptoSnapshot(
        const std::string& symbol,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch crypto snapshots for multiple symbols.
     */
    std::pair<Status, std::map<std::string, CryptoSnapshot>> getCryptoSnapshots(
        const std::vector<std::string>& symbols,
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch historical crypto bars.
     * 
     * @param symbols Crypto symbols
     * @param start Start time (RFC3339 format)
     * @param end End time (RFC3339 format)
     * @param timeframe Timeframe (e.g., "1Min", "1Hour", "1Day")
     * @param limit Maximum number of bars
     * @param page_token Pagination token
     * @param feed Crypto feed
     */
    std::pair<Status, CryptoBars> getCryptoBars(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        const std::string& timeframe = "1Day",
        unsigned int limit = 1000,
        const std::string& page_token = "",
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch historical crypto trades.
     */
    std::pair<Status, CryptoTrades> getCryptoTrades(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "",
        CryptoFeed feed = CryptoFeed::US) const;

    /**
     * @brief Fetch historical crypto quotes.
     */
    std::pair<Status, CryptoQuotes> getCryptoQuotes(
        const std::vector<std::string>& symbols,
        const std::string& start = "",
        const std::string& end = "",
        unsigned int limit = 1000,
        const std::string& page_token = "",
        CryptoFeed feed = CryptoFeed::US) const;

    // Legacy aliases for backward compatibility
    std::pair<Status, LatestTrade> getLastTrade(const std::string& symbol) const { return getLatestTrade(symbol); }
    std::pair<Status, LatestQuote> getLastQuote(const std::string& symbol) const { return getLatestQuote(symbol); }

private:
    Environment environment_;
};

}  // namespace alpaca::markets
