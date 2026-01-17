#pragma once

#include <alpaca/markets/models/status.hpp>

#include <string>

namespace alpaca::markets {

/**
 * @brief The direction to display orders in when enumerating them.
 */
enum class OrderDirection {
    Ascending,
    Descending,
};

/**
 * @brief A helper to convert an OrderDirection to a string
 */
std::string orderDirectionToString(OrderDirection direction);

/**
 * @brief When you submit an order, you may be buying or selling.
 */
enum class OrderSide {
    Buy,
    Sell,
};

/**
 * @brief A helper to convert an OrderSide to a string
 */
std::string orderSideToString(OrderSide side);

/**
 * @brief When you submit an order, you can choose a supported order type.
 *
 * For more information on supported order types, see:
 * https://alpaca.markets/docs/trading-on-alpaca/orders/#order-types
 */
enum class OrderType {
    Market,
    Limit,
    Stop,
    StopLimit,
    TrailingStop,
};

/**
 * @brief A helper to convert an OrderType to a string
 */
std::string orderTypeToString(OrderType type);

/**
 * @brief Alpaca supports several Time-In-Force designations
 *
 * For more information on the supported designations, see:
 * https://alpaca.markets/docs/trading-on-alpaca/orders/#time-in-force
 */
enum class OrderTimeInForce {
    Day,
    GoodUntilCanceled,
    OPG,
    CLS,
    ImmediateOrCancel,
    FillOrKill,
};

/**
 * @brief A helper to convert an OrderTimeInForce to a string
 */
std::string orderTimeInForceToString(OrderTimeInForce tif);

/**
 * @brief The class of the order
 *
 * For details of non-simple order classes, please see:
 * https://alpaca.markets/docs/trading-on-alpaca/orders/#bracket-orders
 */
enum class OrderClass {
    Simple,
    Bracket,
    OneCancelsOther,
    OneTriggersOther,
    MultiLeg,
};

/**
 * @brief A helper to convert an OrderClass to a string
 */
std::string orderClassToString(OrderClass order_class);

/**
 * @brief Position intent for options orders
 */
enum class PositionIntent {
    BuyToOpen,
    BuyToClose,
    SellToOpen,
    SellToClose,
};

/**
 * @brief A helper to convert a PositionIntent to a string
 */
std::string positionIntentToString(PositionIntent intent);

/**
 * @brief Additional parameters for take-profit leg of advanced orders
 */
struct TakeProfitParams {
    /// Required for bracket orders
    std::string limitPrice;
};

/**
 * @brief Additional parameters for stop-loss leg of advanced orders
 */
struct StopLossParams {
    /// Required for bracket orders
    std::string stopPrice;
    /// The stop-loss order becomes a stop-limit order if specified
    std::string limitPrice;
};

/**
 * @brief A type representing an Alpaca order.
 */
class Order {
public:
    /**
     * @brief A method for deserializing JSON into the current object state.
     *
     * @param json The JSON string
     *
     * @return a Status indicating the success or failure of the operation.
     */
    Status fromJSON(const std::string& json);

public:
    std::string asset_class;
    std::string asset_id;
    std::string canceled_at;
    std::string client_order_id;
    std::string created_at;
    std::string expired_at;
    bool extended_hours = false;
    std::string failed_at;
    std::string filled_at;
    std::string filled_avg_price;
    std::string filled_qty;
    std::string id;
    bool legs = false;
    std::string limit_price;
    std::string qty;
    std::string notional;  // For dollar-based/notional orders
    std::string side;
    std::string status;
    std::string stop_price;
    std::string trail_price;     // For trailing stop orders
    std::string trail_percent;   // For trailing stop orders
    std::string hwm;             // High water mark for trailing stops
    std::string submitted_at;
    std::string symbol;
    std::string time_in_force;
    std::string type;
    std::string updated_at;
};

}  // namespace alpaca::markets
