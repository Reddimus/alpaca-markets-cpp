#pragma once

#include <alpaca/markets/models/account.hpp>
#include <alpaca/markets/models/asset.hpp>
#include <alpaca/markets/models/bars.hpp>
#include <alpaca/markets/models/calendar.hpp>
#include <alpaca/markets/models/clock.hpp>
#include <alpaca/markets/models/order.hpp>
#include <alpaca/markets/models/portfolio.hpp>
#include <alpaca/markets/models/position.hpp>
#include <alpaca/markets/models/quote.hpp>
#include <alpaca/markets/models/status.hpp>
#include <alpaca/markets/models/trade.hpp>
#include <alpaca/markets/models/watchlist.hpp>
#include <alpaca/markets/rest/config.hpp>

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
                                         StopLossParams* stop_loss_params = nullptr) const;

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

    // Legacy aliases for backward compatibility
    std::pair<Status, LatestTrade> getLastTrade(const std::string& symbol) const { return getLatestTrade(symbol); }
    std::pair<Status, LatestQuote> getLastQuote(const std::string& symbol) const { return getLatestQuote(symbol); }

private:
    Environment environment_;
};

}  // namespace alpaca::markets
