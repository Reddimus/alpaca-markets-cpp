#include <alpaca/markets/client.hpp>

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <httplib.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "../detail/json.hpp"

namespace alpaca::markets {

namespace {
const char* kJSONContentType = "application/json";

httplib::Headers makeHeaders(const Environment& environment) {
    return {
        {"APCA-API-KEY-ID", environment.getAPIKeyID()},
        {"APCA-API-SECRET-KEY", environment.getAPISecretKey()},
    };
}

// ----- Outer-envelope walk helpers ----------------------------------------
// These wrap the dispatcher pattern that the old RapidJSON path implemented
// inline: parse the top-level body, drill into a nested object/array, hand
// each element's JSON to the corresponding model's fromJSON(string).

template <class T>
Status walkArrayInto(const glz::generic& root, std::vector<T>& out) {
    if (!root.is_array()) {
        return Status(1, "Expected JSON array at root");
    }
    for (const glz::generic& elem : root.get_array()) {
        T item;
        if (Status s = item.fromJSON(json_detail::write(elem)); !s.ok()) {
            return s;
        }
        out.push_back(std::move(item));
    }
    return Status();
}

template <class T>
Status walkArrayInto(const glz::generic& root, std::string_view key, std::vector<T>& out) {
    if (!root.is_object()) {
        return Status();  // nothing to do; preserve previous lenient behaviour
    }
    const glz::generic::object_t& obj = root.get_object();
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return Status();
    }
    for (const glz::generic& elem : it->second.get_array()) {
        T item;
        if (Status s = item.fromJSON(json_detail::write(elem)); !s.ok()) {
            return s;
        }
        out.push_back(std::move(item));
    }
    return Status();
}

template <class T>
Status walkObjectIntoMap(const glz::generic& root, std::string_view key, std::map<std::string, T>& out) {
    if (!root.is_object()) {
        return Status();
    }
    const glz::generic::object_t& obj = root.get_object();
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_object()) {
        return Status();
    }
    for (const auto& [sym, val] : it->second.get_object()) {
        T item;
        if (Status s = item.fromJSON(json_detail::write(val)); !s.ok()) {
            return s;
        }
        out[sym] = std::move(item);
    }
    return Status();
}

// Extract a nested object-typed field's JSON string back out, if present
// and an object.
std::optional<std::string> extractSubObjectJson(const glz::generic& root, std::string_view key) {
    if (!root.is_object()) {
        return std::nullopt;
    }
    const glz::generic::object_t& obj = root.get_object();
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_object()) {
        return std::nullopt;
    }
    return json_detail::write(it->second);
}

bool extractStringField(const glz::generic& root, std::string_view key, std::string& out) {
    if (!root.is_object()) {
        return false;
    }
    return json_detail::obj_get_string(root.get_object(), key, out);
}

/**
 * @brief Parse an API error from a non-200 HTTP response.
 *
 * Alpaca API error responses typically have the format:
 * {"code": 40010000, "message": "error description"}
 *
 * @param status_code HTTP status code
 * @param body Response body (JSON)
 * @return APIError with parsed details or generic error
 */
APIError parseAPIError(int status_code, const std::string& body) {
    int api_code = 0;
    std::string message = body;

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, body); !ec && root.is_object()) {
        const glz::generic::object_t& obj = root.get_object();
        int parsed_code = 0;
        if (json_detail::obj_get_int(obj, "code", parsed_code)) {
            api_code = parsed_code;
        }
        std::string parsed_msg;
        if (json_detail::obj_get_string(obj, "message", parsed_msg)) {
            message = parsed_msg;
        }
    }

    return APIError(status_code, api_code, message, body);
}

/**
 * @brief Create a Status from an API error response.
 */
Status makeErrorStatus(const std::string& endpoint, int status_code, const std::string& body) {
    APIError err = parseAPIError(status_code, body);
    std::ostringstream ss;
    ss << "Call to " << endpoint << " failed: " << err.what();
    return Status(1, ss.str());
}
}  // namespace

Client::Client(Environment& environment) {
    if (!environment.hasBeenParsed()) {
        if (Status s = environment.parse(); !s.ok()) {
            std::cerr << "Error parsing the environment: " << s.getMessage() << std::endl;
        }
    }
    environment_ = environment;
}

// ==================== Account ====================

std::pair<Status, Account> Client::getAccount() const {
    Account account;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get("/v2/account", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/account returned an empty response"), account);
    }

    if (resp->status != 200) {
        return std::make_pair(makeErrorStatus("/v2/account", resp->status, resp->body), account);
    }

    return std::make_pair(account.fromJSON(resp->body), account);
}

std::pair<Status, AccountConfigurations> Client::getAccountConfigurations() const {
    AccountConfigurations account_configurations;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get("/v2/account/configurations", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/account/configurations returned an empty response"),
                              account_configurations);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/account/configurations returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), account_configurations);
    }

    return std::make_pair(account_configurations.fromJSON(resp->body), account_configurations);
}

std::pair<Status, AccountConfigurations> Client::updateAccountConfigurations(bool no_shorting,
                                                                             const std::string& dtbp_check,
                                                                             const std::string& trade_confirm_email,
                                                                             bool suspend_trade) const {
    AccountConfigurations account_configurations;

    // Build the patch body via Glaze. Using a glz::generic ordered_map
    // matches the field order the previous RapidJSON Writer emitted; the
    // server doesn't care about key order but log diffs stay readable.
    glz::generic body_obj = glz::generic::object_t{};
    glz::generic::object_t& obj = body_obj.get_object();
    obj["no_shorting"] = no_shorting;
    obj["dtbp_check"] = dtbp_check;
    obj["trade_confirm_email"] = trade_confirm_email;
    obj["suspend_trade"] = suspend_trade;
    std::string body;
    (void)glz::write_json(body_obj, body);

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp =
        client.Patch("/v2/account/configurations", makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/account/configurations returned an empty response"),
                              account_configurations);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/account/configurations returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), account_configurations);
    }

    return std::make_pair(account_configurations.fromJSON(resp->body), account_configurations);
}

std::pair<Status, std::vector<std::variant<TradeActivity, NonTradeActivity>>> Client::getAccountActivity(
    const std::vector<std::string>& activity_types) const {
    std::vector<std::variant<TradeActivity, NonTradeActivity>> activities;

    std::string url = "/v2/account/activities";
    if (!activity_types.empty()) {
        std::string query_string;
        for (size_t i = 0; i < activity_types.size(); ++i) {
            query_string += activity_types[i];
            query_string += ",";
        }
        query_string.pop_back();
        url += "?activity_types=" + query_string;
    }

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), activities);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), activities);
    }

    // Activities is an array of mixed-typed records discriminated by the
    // "activity_type" field. Walk the array element-by-element, dispatch
    // to the appropriate model. Glaze's tagged-union support would fit
    // here once we hoist the activity_type → model mapping into a
    // glz::meta with a discriminator tag, but the activity_type strings
    // are open-ended ("FILL", "DIV", "INT", "JNL", ...) and only "FILL"
    // dispatches to TradeActivity, so the explicit switch stays cheaper.
    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing activities JSON"), activities);
    }
    if (!root.is_array()) {
        return std::make_pair(Status(1, "Expected array of activities"), activities);
    }
    for (const glz::generic& elem : root.get_array()) {
        if (!elem.is_object()) {
            return std::make_pair(Status(1, "Activity entry wasn't an object"), activities);
        }
        std::string activity_type;
        if (!json_detail::obj_get_string(elem.get_object(), "activity_type", activity_type)) {
            return std::make_pair(Status(1, "Activity didn't have activity_type attribute"), activities);
        }
        std::string entry_json = json_detail::write(elem);
        if (activity_type == "FILL") {
            TradeActivity activity;
            if (Status status = activity.fromJSON(entry_json); !status.ok()) {
                return std::make_pair(status, activities);
            }
            activities.push_back(activity);
        } else {
            NonTradeActivity activity;
            if (Status status = activity.fromJSON(entry_json); !status.ok()) {
                return std::make_pair(status, activities);
            }
            activities.push_back(activity);
        }
    }

    return std::make_pair(Status(), activities);
}

// ==================== Orders ====================

std::pair<Status, Order> Client::getOrder(const std::string& id, bool nested) const {
    Order order;

    std::string url = "/v2/orders/" + id;
    if (nested) {
        url += "?nested=true";
    }

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), order);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

std::pair<Status, Order> Client::getOrderByClientOrderID(const std::string& client_order_id) const {
    Order order;

    std::string url = "/v2/orders:by_client_order_id?client_order_id=" + client_order_id;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), order);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

std::pair<Status, std::vector<Order>> Client::getOrders(ActionStatus status, int limit, const std::string& after,
                                                        const std::string& until, OrderDirection direction,
                                                        bool nested) const {
    std::vector<Order> orders;

    httplib::Params params{
        {"status", actionStatusToString(status)},
        {"limit", std::to_string(limit)},
        {"direction", orderDirectionToString(direction)},
    };
    if (!after.empty()) {
        params.insert({"after", after});
    }
    if (!until.empty()) {
        params.insert({"until", until});
    }
    if (nested) {
        params.insert({"nested", "true"});
    }
    std::string query_string = httplib::detail::params_to_query_str(params);
    httplib::SSLClient client(environment_.getTradingHost());
    std::string url = "/v2/orders?" + query_string;
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), orders);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), orders);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing orders JSON"), orders);
    }
    if (Status s = walkArrayInto(root, orders); !s.ok()) {
        return std::make_pair(s, orders);
    }

    return std::make_pair(Status(), orders);
}

std::pair<Status, Order> Client::submitOrder(const std::string& symbol, int quantity, OrderSide side, OrderType type,
                                             OrderTimeInForce tif, const std::string& limit_price,
                                             const std::string& stop_price, bool extended_hours,
                                             const std::string& client_order_id, OrderClass order_class,
                                             TakeProfitParams* take_profit_params, StopLossParams* stop_loss_params,
                                             const std::string& trail_price, const std::string& trail_percent) const {
    Order order;

    glz::generic body_obj = glz::generic::object_t{};
    glz::generic::object_t& obj = body_obj.get_object();
    obj["symbol"] = symbol;
    obj["qty"] = static_cast<double>(quantity);  // serialised as integer below; v2 API accepts numeric qty for shares
    obj["side"] = orderSideToString(side);
    obj["type"] = orderTypeToString(type);
    obj["time_in_force"] = orderTimeInForceToString(tif);

    if (!limit_price.empty()) {
        obj["limit_price"] = limit_price;
    }
    if (!stop_price.empty()) {
        obj["stop_price"] = stop_price;
    }
    if (!trail_price.empty()) {
        obj["trail_price"] = trail_price;
    }
    if (!trail_percent.empty()) {
        obj["trail_percent"] = trail_percent;
    }
    if (extended_hours) {
        obj["extended_hours"] = extended_hours;
    }
    if (!client_order_id.empty()) {
        obj["client_order_id"] = client_order_id;
    }
    if (order_class != OrderClass::Simple) {
        obj["order_class"] = orderClassToString(order_class);
    }

    if (take_profit_params != nullptr) {
        glz::generic tp = glz::generic::object_t{};
        glz::generic::object_t& tp_obj = tp.get_object();
        if (!take_profit_params->limitPrice.empty()) {
            tp_obj["limit_price"] = take_profit_params->limitPrice;
        }
        obj["take_profit"] = std::move(tp);
    }

    if (stop_loss_params != nullptr) {
        glz::generic sl = glz::generic::object_t{};
        glz::generic::object_t& sl_obj = sl.get_object();
        if (!stop_loss_params->limitPrice.empty()) {
            sl_obj["limit_price"] = stop_loss_params->limitPrice;
        }
        if (!stop_loss_params->stopPrice.empty()) {
            sl_obj["stop_price"] = stop_loss_params->stopPrice;
        }
        obj["stop_loss"] = std::move(sl);
    }

    std::string body;
    (void)glz::write_json(body_obj, body);

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Post("/v2/orders", makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/orders returned an empty response"), order);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/orders returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

std::pair<Status, Order> Client::submitNotionalOrder(const std::string& symbol, const std::string& notional,
                                                     OrderSide side, OrderType type, OrderTimeInForce tif,
                                                     const std::string& limit_price, bool extended_hours,
                                                     const std::string& client_order_id) const {
    Order order;

    glz::generic body_obj = glz::generic::object_t{};
    glz::generic::object_t& obj = body_obj.get_object();
    obj["symbol"] = symbol;
    obj["notional"] = notional;
    obj["side"] = orderSideToString(side);
    obj["type"] = orderTypeToString(type);
    obj["time_in_force"] = orderTimeInForceToString(tif);
    if (!limit_price.empty()) {
        obj["limit_price"] = limit_price;
    }
    if (extended_hours) {
        obj["extended_hours"] = extended_hours;
    }
    if (!client_order_id.empty()) {
        obj["client_order_id"] = client_order_id;
    }
    std::string body;
    (void)glz::write_json(body_obj, body);

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Post("/v2/orders", makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/orders returned an empty response"), order);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/orders returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

std::pair<Status, Order> Client::replaceOrder(const std::string& id, int quantity, OrderTimeInForce tif,
                                              const std::string& limit_price, const std::string& stop_price,
                                              const std::string& client_order_id) const {
    Order order;

    glz::generic body_obj = glz::generic::object_t{};
    glz::generic::object_t& obj = body_obj.get_object();
    obj["qty"] = static_cast<double>(quantity);
    obj["time_in_force"] = orderTimeInForceToString(tif);
    if (!limit_price.empty()) {
        obj["limit_price"] = limit_price;
    }
    if (!stop_price.empty()) {
        obj["stop_price"] = stop_price;
    }
    if (!client_order_id.empty()) {
        obj["client_order_id"] = client_order_id;
    }
    std::string body;
    (void)glz::write_json(body_obj, body);

    std::string url = "/v2/orders/" + id;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Patch(url.c_str(), makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), order);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

std::pair<Status, std::vector<Order>> Client::cancelOrders() const {
    std::vector<Order> orders;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Delete("/v2/orders", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/orders returned an empty response"), orders);
    }

    if (resp->status != 200 && resp->status != 207) {
        std::ostringstream ss;
        ss << "Call to /v2/orders returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), orders);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing orders JSON"), orders);
    }
    if (Status s = walkArrayInto(root, orders); !s.ok()) {
        return std::make_pair(s, orders);
    }

    return std::make_pair(Status(), orders);
}

std::pair<Status, Order> Client::cancelOrder(const std::string& id) const {
    Order order;

    httplib::SSLClient client(environment_.getTradingHost());
    std::string url = "/v2/orders/" + id;
    httplib::Result resp = client.Delete(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), order);
    }

    if (resp->status == 204) {
        return getOrder(id);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), order);
    }

    return std::make_pair(order.fromJSON(resp->body), order);
}

// ==================== Positions ====================

std::pair<Status, std::vector<Position>> Client::getPositions() const {
    std::vector<Position> positions;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get("/v2/positions", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/positions returned an empty response"), positions);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/positions returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), positions);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing positions JSON"), positions);
    }
    if (Status s = walkArrayInto(root, positions); !s.ok()) {
        return std::make_pair(s, positions);
    }

    return std::make_pair(Status(), positions);
}

std::pair<Status, Position> Client::getPosition(const std::string& symbol) const {
    Position position;

    std::string url = "/v2/positions/" + symbol;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), position);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), position);
    }

    return std::make_pair(position.fromJSON(resp->body), position);
}

std::pair<Status, std::vector<Position>> Client::closePositions() const {
    std::vector<Position> positions;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Delete("/v2/positions", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/positions returned an empty response"), positions);
    }

    if (resp->status != 200 && resp->status != 207) {
        std::ostringstream ss;
        ss << "Call to /v2/positions returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), positions);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing positions JSON"), positions);
    }
    if (Status s = walkArrayInto(root, positions); !s.ok()) {
        return std::make_pair(s, positions);
    }

    return std::make_pair(Status(), positions);
}

std::pair<Status, Position> Client::closePosition(const std::string& symbol) const {
    Position position;

    httplib::SSLClient client(environment_.getTradingHost());
    std::string url = "/v2/positions/" + symbol;
    httplib::Result resp = client.Delete(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), position);
    }

    if (resp->status == 204) {
        return getPosition(symbol);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), position);
    }

    return std::make_pair(position.fromJSON(resp->body), position);
}

// ==================== Assets ====================

std::pair<Status, std::vector<Asset>> Client::getAssets(ActionStatus asset_status, AssetClass asset_class) const {
    std::vector<Asset> assets;

    httplib::Params params{
        {"status", actionStatusToString(asset_status)},
        {"asset_class", assetClassToString(asset_class)},
    };
    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/assets?" + query_string;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), assets);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), assets);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing assets JSON"), assets);
    }
    if (Status s = walkArrayInto(root, assets); !s.ok()) {
        return std::make_pair(s, assets);
    }

    return std::make_pair(Status(), assets);
}

std::pair<Status, Asset> Client::getAsset(const std::string& symbol) const {
    Asset asset;

    std::string url = "/v2/assets/" + symbol;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), asset);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), asset);
    }

    return std::make_pair(asset.fromJSON(resp->body), asset);
}

// ==================== Clock & Calendar ====================

std::pair<Status, Clock> Client::getClock() const {
    Clock clock;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get("/v2/clock", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/clock returned an empty response"), clock);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/clock returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), clock);
    }

    return std::make_pair(clock.fromJSON(resp->body), clock);
}

std::pair<Status, std::vector<Date>> Client::getCalendar(const std::string& start, const std::string& end) const {
    std::vector<Date> dates;

    std::string url = "/v2/calendar?start=" + start + "&end=" + end;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), dates);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), dates);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing calendar JSON"), dates);
    }
    if (Status s = walkArrayInto(root, dates); !s.ok()) {
        return std::make_pair(s, dates);
    }

    return std::make_pair(Status(), dates);
}

// ==================== Watchlists ====================

std::pair<Status, std::vector<Watchlist>> Client::getWatchlists() const {
    std::vector<Watchlist> watchlists;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get("/v2/watchlists", makeHeaders(environment_));
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/watchlists returned an empty response"), watchlists);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/watchlists returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlists);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing watchlists JSON"), watchlists);
    }
    if (Status s = walkArrayInto(root, watchlists); !s.ok()) {
        return std::make_pair(s, watchlists);
    }

    return std::make_pair(Status(), watchlists);
}

std::pair<Status, Watchlist> Client::getWatchlist(const std::string& id) const {
    Watchlist watchlist;

    std::string url = "/v2/watchlists/" + id;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    return std::make_pair(watchlist.fromJSON(resp->body), watchlist);
}

namespace {
// Helper: build {"name": <name>, "symbols": [<symbols>]} body
std::string makeWatchlistBody(const std::string& name, const std::vector<std::string>& symbols) {
    glz::generic body_obj = glz::generic::object_t{};
    glz::generic::object_t& obj = body_obj.get_object();
    obj["name"] = name;
    glz::generic arr = glz::generic::array_t{};
    glz::generic::array_t& a = arr.get_array();
    for (const std::string& symbol : symbols) {
        a.emplace_back(symbol);
    }
    obj["symbols"] = std::move(arr);
    std::string out;
    (void)glz::write_json(body_obj, out);
    return out;
}
}  // namespace

std::pair<Status, Watchlist> Client::createWatchlist(const std::string& name,
                                                     const std::vector<std::string>& symbols) const {
    Watchlist watchlist;

    std::string body = makeWatchlistBody(name, symbols);

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Post("/v2/watchlists", makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        return std::make_pair(Status(1, "Call to /v2/watchlists returned an empty response"), watchlist);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to /v2/watchlists returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    return std::make_pair(watchlist.fromJSON(resp->body), watchlist);
}

std::pair<Status, Watchlist> Client::updateWatchlist(const std::string& id, const std::string& name,
                                                     const std::vector<std::string>& symbols) const {
    Watchlist watchlist;

    std::string body = makeWatchlistBody(name, symbols);

    std::string url = "/v2/watchlists/" + id;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Put(url.c_str(), makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    return std::make_pair(watchlist.fromJSON(resp->body), watchlist);
}

Status Client::deleteWatchlist(const std::string& id) const {
    std::string url = "/v2/watchlists/" + id;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Delete(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return Status(1, ss.str());
    }

    if (resp->status != 200 && resp->status != 204) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return Status(1, ss.str());
    }
    return Status();
}

std::pair<Status, Watchlist> Client::addSymbolToWatchlist(const std::string& id, const std::string& symbol) const {
    Watchlist watchlist;

    glz::generic body_obj = glz::generic::object_t{};
    body_obj.get_object()["symbol"] = symbol;
    std::string body;
    (void)glz::write_json(body_obj, body);

    std::string url = "/v2/watchlists/" + id;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Post(url.c_str(), makeHeaders(environment_), body, kJSONContentType);
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    return std::make_pair(watchlist.fromJSON(resp->body), watchlist);
}

std::pair<Status, Watchlist> Client::removeSymbolFromWatchlist(const std::string& id, const std::string& symbol) const {
    Watchlist watchlist;

    std::string url = "/v2/watchlists/" + id + "/" + symbol;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Delete(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), watchlist);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), watchlist);
    }
    return std::make_pair(watchlist.fromJSON(resp->body), watchlist);
}

// ==================== Portfolio ====================

std::pair<Status, PortfolioHistory> Client::getPortfolioHistory(const std::string& period, const std::string& timeframe,
                                                                const std::string& date_end,
                                                                bool extended_hours) const {
    PortfolioHistory portfolio_history;

    std::string query_string;

    if (!period.empty()) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += "period=" + period;
    }

    if (!timeframe.empty()) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += "timeframe=" + timeframe;
    }

    if (!date_end.empty()) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += "date_end=" + date_end;
    }

    if (extended_hours) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += "extended_hours=true";
    }

    if (!query_string.empty()) {
        query_string = "?" + query_string;
    }

    std::string url = "/v2/account/portfolio/history" + query_string;
    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), portfolio_history);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), portfolio_history);
    }

    return std::make_pair(portfolio_history.fromJSON(resp->body), portfolio_history);
}

// ==================== Market Data (v2) ====================

std::pair<Status, Bars> Client::getBars(const std::vector<std::string>& symbols, const std::string& start,
                                        const std::string& end, const std::string& timeframe, unsigned int limit,
                                        const std::string& page_token) const {
    Bars bars;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{
        {"symbols", symbols_string},
        {"timeframe", timeframe},
        {"limit", std::to_string(limit)},
    };
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }
    std::string query_string = httplib::detail::params_to_query_str(params);

    // Market Data API v2 endpoint
    std::string url = "/v2/stocks/bars?" + query_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), bars);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), bars);
    }

    return std::make_pair(bars.fromJSON(resp->body), bars);
}

std::pair<Status, LatestTrade> Client::getLatestTrade(const std::string& symbol) const {
    LatestTrade latest_trade;

    // Market Data API v2 endpoint
    std::string url = "/v2/stocks/" + symbol + "/trades/latest";

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), latest_trade);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), latest_trade);
    }

    return std::make_pair(latest_trade.fromJSON(resp->body), latest_trade);
}

std::pair<Status, LatestQuote> Client::getLatestQuote(const std::string& symbol) const {
    LatestQuote latest_quote;

    // Market Data API v2 endpoint
    std::string url = "/v2/stocks/" + symbol + "/quotes/latest";

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), latest_quote);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), latest_quote);
    }

    return std::make_pair(latest_quote.fromJSON(resp->body), latest_quote);
}

std::pair<Status, std::map<std::string, Trade>> Client::getLatestTrades(const std::vector<std::string>& symbols) const {
    std::map<std::string, Trade> trades;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = "/v2/stocks/trades/latest?symbols=" + symbols_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), trades);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), trades);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest trades JSON"), trades);
    }
    if (Status s = walkObjectIntoMap(root, "trades", trades); !s.ok()) {
        return std::make_pair(s, trades);
    }

    return std::make_pair(Status(), trades);
}

std::pair<Status, std::map<std::string, Quote>> Client::getLatestQuotes(const std::vector<std::string>& symbols) const {
    std::map<std::string, Quote> quotes;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = "/v2/stocks/quotes/latest?symbols=" + symbols_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), quotes);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), quotes);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest quotes JSON"), quotes);
    }
    if (Status s = walkObjectIntoMap(root, "quotes", quotes); !s.ok()) {
        return std::make_pair(s, quotes);
    }

    return std::make_pair(Status(), quotes);
}

// ==================== Corporate Actions ====================

std::pair<Status, std::vector<Announcement>> Client::getAnnouncements(
    const std::vector<std::string>& ca_types, const std::string& since, const std::string& until,
    const std::string& symbol, const std::string& cusip, const std::string& date_type) const {
    std::vector<Announcement> announcements;

    std::string query_string;

    if (!ca_types.empty()) {
        std::string types_string;
        for (size_t i = 0; i < ca_types.size(); ++i) {
            types_string += ca_types[i];
            if (i < ca_types.size() - 1) {
                types_string += ",";
            }
        }
        query_string += "ca_types=" + types_string;
    }

    if (!since.empty()) {
        if (!query_string.empty())
            query_string += "&";
        query_string += "since=" + since;
    }

    if (!until.empty()) {
        if (!query_string.empty())
            query_string += "&";
        query_string += "until=" + until;
    }

    if (!symbol.empty()) {
        if (!query_string.empty())
            query_string += "&";
        query_string += "symbol=" + symbol;
    }

    if (!cusip.empty()) {
        if (!query_string.empty())
            query_string += "&";
        query_string += "cusip=" + cusip;
    }

    if (!date_type.empty()) {
        if (!query_string.empty())
            query_string += "&";
        query_string += "date_type=" + date_type;
    }

    std::string url = "/v2/corporate_actions/announcements";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), announcements);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), announcements);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing announcements JSON"), announcements);
    }
    if (!root.is_array()) {
        return std::make_pair(Status(1, "Expected array of announcements"), announcements);
    }
    if (Status s = walkArrayInto(root, announcements); !s.ok()) {
        return std::make_pair(s, announcements);
    }

    return std::make_pair(Status(), announcements);
}

std::pair<Status, Announcement> Client::getAnnouncement(const std::string& id) const {
    Announcement announcement;

    std::string url = "/v2/corporate_actions/announcements/" + id;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), announcement);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), announcement);
    }

    return std::make_pair(announcement.fromJSON(resp->body), announcement);
}

// ==================== Options ====================

std::pair<Status, OptionContracts> Client::getOptionContracts(
    const std::string& underlying_symbols, const std::string& status, const std::string& expiration_date,
    const std::string& expiration_date_gte, const std::string& expiration_date_lte, const std::string& root_symbol,
    const std::string& type, const std::string& style, const std::string& strike_price_gte,
    const std::string& strike_price_lte, unsigned int limit, const std::string& page_token) const {
    OptionContracts contracts;

    httplib::Params params;
    if (!underlying_symbols.empty()) {
        params.insert({"underlying_symbols", underlying_symbols});
    }
    if (!status.empty()) {
        params.insert({"status", status});
    }
    if (!expiration_date.empty()) {
        params.insert({"expiration_date", expiration_date});
    }
    if (!expiration_date_gte.empty()) {
        params.insert({"expiration_date_gte", expiration_date_gte});
    }
    if (!expiration_date_lte.empty()) {
        params.insert({"expiration_date_lte", expiration_date_lte});
    }
    if (!root_symbol.empty()) {
        params.insert({"root_symbol", root_symbol});
    }
    if (!type.empty()) {
        params.insert({"type", type});
    }
    if (!style.empty()) {
        params.insert({"style", style});
    }
    if (!strike_price_gte.empty()) {
        params.insert({"strike_price_gte", strike_price_gte});
    }
    if (!strike_price_lte.empty()) {
        params.insert({"strike_price_lte", strike_price_lte});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/options/contracts";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), contracts);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), contracts);
    }

    return std::make_pair(contracts.fromJSON(resp->body), contracts);
}

std::pair<Status, OptionContract> Client::getOptionContract(const std::string& symbol_or_id) const {
    OptionContract contract;

    std::string url = "/v2/options/contracts/" + symbol_or_id;

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), contract);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), contract);
    }

    return std::make_pair(contract.fromJSON(resp->body), contract);
}

// ==================== Market Data - Snapshots ====================

std::pair<Status, Snapshot> Client::getSnapshot(const std::string& symbol) const {
    Snapshot snapshot;

    std::string url = "/v2/stocks/" + symbol + "/snapshot";

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), snapshot);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), snapshot);
    }

    return std::make_pair(snapshot.fromJSON(resp->body), snapshot);
}

std::pair<Status, std::map<std::string, Snapshot>> Client::getSnapshots(const std::vector<std::string>& symbols) const {
    std::map<std::string, Snapshot> snapshots;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = "/v2/stocks/snapshots?symbols=" + symbols_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), snapshots);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), snapshots);
    }

    // Response is object keyed by symbol (no "snapshots" wrapper here).
    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing snapshots JSON"), snapshots);
    }
    if (root.is_object()) {
        for (const auto& [sym, val] : root.get_object()) {
            Snapshot snapshot;
            if (Status s = snapshot.fromJSON(json_detail::write(val)); !s.ok()) {
                return std::make_pair(s, snapshots);
            }
            snapshots[sym] = std::move(snapshot);
        }
    }

    return std::make_pair(Status(), snapshots);
}

// ==================== Market Data - Latest Bars ====================

std::pair<Status, Bar> Client::getLatestBar(const std::string& symbol) const {
    Bar bar;

    std::string url = "/v2/stocks/" + symbol + "/bars/latest";

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), bar);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), bar);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest bar JSON"), bar);
    }
    if (std::optional<std::string> sub = extractSubObjectJson(root, "bar"); sub.has_value()) {
        return std::make_pair(bar.fromJSON(*sub), bar);
    }

    return std::make_pair(Status(1, "Response missing 'bar' field"), bar);
}

std::pair<Status, std::map<std::string, Bar>> Client::getLatestBars(const std::vector<std::string>& symbols) const {
    std::map<std::string, Bar> bars;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = "/v2/stocks/bars/latest?symbols=" + symbols_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), bars);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), bars);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest bars JSON"), bars);
    }
    if (Status s = walkObjectIntoMap(root, "bars", bars); !s.ok()) {
        return std::make_pair(s, bars);
    }

    return std::make_pair(Status(), bars);
}

// ==================== Market Data - Historical Trades/Quotes ====================

std::pair<Status, std::pair<std::vector<Trade>, std::string>> Client::getTrades(const std::string& symbol,
                                                                                const std::string& start,
                                                                                const std::string& end,
                                                                                unsigned int limit,
                                                                                const std::string& page_token) const {
    std::vector<Trade> trades;
    std::string next_page_token;

    httplib::Params params;
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/" + symbol + "/trades";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), std::make_pair(trades, next_page_token));
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), std::make_pair(trades, next_page_token));
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing trades JSON"),
                              std::make_pair(trades, next_page_token));
    }
    if (Status s = walkArrayInto(root, "trades", trades); !s.ok()) {
        return std::make_pair(s, std::make_pair(trades, next_page_token));
    }
    extractStringField(root, "next_page_token", next_page_token);

    return std::make_pair(Status(), std::make_pair(trades, next_page_token));
}

std::pair<Status, std::pair<std::vector<Quote>, std::string>> Client::getQuotes(const std::string& symbol,
                                                                                const std::string& start,
                                                                                const std::string& end,
                                                                                unsigned int limit,
                                                                                const std::string& page_token) const {
    std::vector<Quote> quotes;
    std::string next_page_token;

    httplib::Params params;
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/" + symbol + "/quotes";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), std::make_pair(quotes, next_page_token));
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), std::make_pair(quotes, next_page_token));
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing quotes JSON"),
                              std::make_pair(quotes, next_page_token));
    }
    if (Status s = walkArrayInto(root, "quotes", quotes); !s.ok()) {
        return std::make_pair(s, std::make_pair(quotes, next_page_token));
    }
    extractStringField(root, "next_page_token", next_page_token);

    return std::make_pair(Status(), std::make_pair(quotes, next_page_token));
}

// ==================== Market Data - Multi-Symbol Historical ====================

std::pair<Status, MultiTrades> Client::getMultiTrades(const std::vector<std::string>& symbols, const std::string& start,
                                                      const std::string& end, unsigned int limit,
                                                      const std::string& page_token) const {
    MultiTrades multi_trades;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/trades?" + query_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), multi_trades);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), multi_trades);
    }

    return std::make_pair(multi_trades.fromJSON(resp->body), multi_trades);
}

std::pair<Status, MultiQuotes> Client::getMultiQuotes(const std::vector<std::string>& symbols, const std::string& start,
                                                      const std::string& end, unsigned int limit,
                                                      const std::string& page_token) const {
    MultiQuotes multi_quotes;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/quotes?" + query_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), multi_quotes);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), multi_quotes);
    }

    return std::make_pair(multi_quotes.fromJSON(resp->body), multi_quotes);
}

// ==================== Market Data - Auctions ====================

std::pair<Status, Auctions> Client::getAuctions(const std::string& symbol, const std::string& start,
                                                const std::string& end, unsigned int limit,
                                                const std::string& page_token) const {
    Auctions auctions;

    httplib::Params params;
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/" + symbol + "/auctions";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), auctions);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), auctions);
    }

    return std::make_pair(auctions.fromJSON(resp->body), auctions);
}

std::pair<Status, Auctions> Client::getMultiAuctions(const std::vector<std::string>& symbols, const std::string& start,
                                                     const std::string& end, unsigned int limit,
                                                     const std::string& page_token) const {
    Auctions auctions;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v2/stocks/auctions?" + query_string;

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), auctions);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), auctions);
    }

    return std::make_pair(auctions.fromJSON(resp->body), auctions);
}

// ==================== Market Data - Corporate Actions ====================

std::pair<Status, CorporateActions> Client::getCorporateActions(const std::vector<std::string>& symbols,
                                                                const std::vector<std::string>& types,
                                                                const std::string& start, const std::string& end,
                                                                unsigned int limit,
                                                                const std::string& page_token) const {
    CorporateActions corporate_actions;

    httplib::Params params;
    if (!symbols.empty()) {
        std::string symbols_string;
        for (size_t i = 0; i < symbols.size(); ++i) {
            symbols_string += symbols[i];
            if (i < symbols.size() - 1) {
                symbols_string += ",";
            }
        }
        params.insert({"symbols", symbols_string});
    }
    if (!types.empty()) {
        std::string types_string;
        for (size_t i = 0; i < types.size(); ++i) {
            types_string += types[i];
            if (i < types.size() - 1) {
                types_string += ",";
            }
        }
        params.insert({"types", types_string});
    }
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v1beta1/corporate-actions";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), corporate_actions);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), corporate_actions);
    }

    return std::make_pair(corporate_actions.fromJSON(resp->body), corporate_actions);
}

// ==================== News API ====================

std::pair<Status, NewsArticles> Client::getNews(const std::vector<std::string>& symbols, const std::string& start,
                                                const std::string& end, unsigned int limit,
                                                const std::string& page_token, bool include_content,
                                                bool exclude_contentless) const {
    NewsArticles news_articles;

    httplib::Params params;
    if (!symbols.empty()) {
        std::string symbols_string;
        for (size_t i = 0; i < symbols.size(); ++i) {
            symbols_string += symbols[i];
            if (i < symbols.size() - 1) {
                symbols_string += ",";
            }
        }
        params.insert({"symbols", symbols_string});
    }
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }
    if (include_content) {
        params.insert({"include_content", "true"});
    }
    if (exclude_contentless) {
        params.insert({"exclude_contentless", "true"});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = "/v1beta1/news";
    if (!query_string.empty()) {
        url += "?" + query_string;
    }

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), news_articles);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), news_articles);
    }

    return std::make_pair(news_articles.fromJSON(resp->body), news_articles);
}

// ==================== Crypto Market Data ====================

namespace {
std::string makeCryptoUrl(const std::string& path, CryptoFeed feed) {
    std::string feed_str = (feed == CryptoFeed::Global) ? "global" : "us";
    return "/v1beta3/crypto/" + feed_str + path;
}

// Walk a {"<key>": {"<symbol>": <obj>}} response, pulling the single-symbol
// payload out into target_model.
template <class T>
Status extractSymbolFromObjectMap(const std::string& body, const std::string& outer_key, const std::string& symbol,
                                  T& target_model, const std::string& not_found_msg) {
    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, body); ec) {
        return Status(1, "Received parse error when deserializing " + outer_key + " JSON");
    }
    if (!root.is_object()) {
        return Status(1, not_found_msg);
    }
    const glz::generic::object_t& root_obj = root.get_object();
    glz::generic::object_t::const_iterator outer_it = root_obj.find(outer_key);
    if (outer_it == root_obj.end() || !outer_it->second.is_object()) {
        return Status(1, not_found_msg);
    }
    const glz::generic::object_t& outer = outer_it->second.get_object();
    glz::generic::object_t::const_iterator sym_it = outer.find(symbol);
    if (sym_it == outer.end() || !sym_it->second.is_object()) {
        return Status(1, not_found_msg);
    }
    return target_model.fromJSON(json_detail::write(sym_it->second));
}
}  // namespace

std::pair<Status, CryptoTrade> Client::getLatestCryptoTrade(const std::string& symbol, CryptoFeed feed) const {
    CryptoTrade trade;

    std::string url = makeCryptoUrl("/latest/trades?symbols=" + symbol, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), trade);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), trade);
    }

    Status s = extractSymbolFromObjectMap(resp->body, "trades", symbol, trade, "Trade not found for symbol");
    return std::make_pair(s, trade);
}

std::pair<Status, std::map<std::string, CryptoTrade>> Client::getLatestCryptoTrades(
    const std::vector<std::string>& symbols, CryptoFeed feed) const {
    std::map<std::string, CryptoTrade> trades;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = makeCryptoUrl("/latest/trades?symbols=" + symbols_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), trades);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), trades);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto trades JSON"), trades);
    }
    if (Status s = walkObjectIntoMap(root, "trades", trades); !s.ok()) {
        return std::make_pair(s, trades);
    }

    return std::make_pair(Status(), trades);
}

std::pair<Status, CryptoQuote> Client::getLatestCryptoQuote(const std::string& symbol, CryptoFeed feed) const {
    CryptoQuote quote;

    std::string url = makeCryptoUrl("/latest/quotes?symbols=" + symbol, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), quote);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), quote);
    }

    Status s = extractSymbolFromObjectMap(resp->body, "quotes", symbol, quote, "Quote not found for symbol");
    return std::make_pair(s, quote);
}

std::pair<Status, std::map<std::string, CryptoQuote>> Client::getLatestCryptoQuotes(
    const std::vector<std::string>& symbols, CryptoFeed feed) const {
    std::map<std::string, CryptoQuote> quotes;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = makeCryptoUrl("/latest/quotes?symbols=" + symbols_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), quotes);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), quotes);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto quotes JSON"), quotes);
    }
    if (Status s = walkObjectIntoMap(root, "quotes", quotes); !s.ok()) {
        return std::make_pair(s, quotes);
    }

    return std::make_pair(Status(), quotes);
}

std::pair<Status, CryptoBar> Client::getLatestCryptoBar(const std::string& symbol, CryptoFeed feed) const {
    CryptoBar bar;

    std::string url = makeCryptoUrl("/latest/bars?symbols=" + symbol, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), bar);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), bar);
    }

    Status s = extractSymbolFromObjectMap(resp->body, "bars", symbol, bar, "Bar not found for symbol");
    return std::make_pair(s, bar);
}

std::pair<Status, std::map<std::string, CryptoBar>> Client::getLatestCryptoBars(const std::vector<std::string>& symbols,
                                                                                CryptoFeed feed) const {
    std::map<std::string, CryptoBar> bars;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = makeCryptoUrl("/latest/bars?symbols=" + symbols_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), bars);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), bars);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto bars JSON"), bars);
    }
    if (Status s = walkObjectIntoMap(root, "bars", bars); !s.ok()) {
        return std::make_pair(s, bars);
    }

    return std::make_pair(Status(), bars);
}

std::pair<Status, CryptoSnapshot> Client::getCryptoSnapshot(const std::string& symbol, CryptoFeed feed) const {
    CryptoSnapshot snapshot;

    std::string url = makeCryptoUrl("/snapshots?symbols=" + symbol, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), snapshot);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), snapshot);
    }

    Status s = extractSymbolFromObjectMap(resp->body, "snapshots", symbol, snapshot, "Snapshot not found for symbol");
    return std::make_pair(s, snapshot);
}

std::pair<Status, std::map<std::string, CryptoSnapshot>> Client::getCryptoSnapshots(
    const std::vector<std::string>& symbols, CryptoFeed feed) const {
    std::map<std::string, CryptoSnapshot> snapshots;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    std::string url = makeCryptoUrl("/snapshots?symbols=" + symbols_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), snapshots);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), snapshots);
    }

    glz::generic root{};
    if (glz::error_ctx ec = glz::read_json(root, resp->body); ec) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto snapshots JSON"), snapshots);
    }
    if (Status s = walkObjectIntoMap(root, "snapshots", snapshots); !s.ok()) {
        return std::make_pair(s, snapshots);
    }

    return std::make_pair(Status(), snapshots);
}

std::pair<Status, CryptoBars> Client::getCryptoBars(const std::vector<std::string>& symbols, const std::string& start,
                                                    const std::string& end, const std::string& timeframe,
                                                    unsigned int limit, const std::string& page_token,
                                                    CryptoFeed feed) const {
    CryptoBars crypto_bars;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}, {"timeframe", timeframe}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = makeCryptoUrl("/bars?" + query_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), crypto_bars);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), crypto_bars);
    }

    return std::make_pair(crypto_bars.fromJSON(resp->body), crypto_bars);
}

std::pair<Status, CryptoTrades> Client::getCryptoTrades(const std::vector<std::string>& symbols,
                                                        const std::string& start, const std::string& end,
                                                        unsigned int limit, const std::string& page_token,
                                                        CryptoFeed feed) const {
    CryptoTrades crypto_trades;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = makeCryptoUrl("/trades?" + query_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), crypto_trades);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), crypto_trades);
    }

    return std::make_pair(crypto_trades.fromJSON(resp->body), crypto_trades);
}

std::pair<Status, CryptoQuotes> Client::getCryptoQuotes(const std::vector<std::string>& symbols,
                                                        const std::string& start, const std::string& end,
                                                        unsigned int limit, const std::string& page_token,
                                                        CryptoFeed feed) const {
    CryptoQuotes crypto_quotes;

    std::string symbols_string;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols_string += symbols[i];
        if (i < symbols.size() - 1) {
            symbols_string += ",";
        }
    }

    httplib::Params params{{"symbols", symbols_string}};
    if (!start.empty()) {
        params.insert({"start", start});
    }
    if (!end.empty()) {
        params.insert({"end", end});
    }
    if (limit > 0) {
        params.insert({"limit", std::to_string(limit)});
    }
    if (!page_token.empty()) {
        params.insert({"page_token", page_token});
    }

    std::string query_string = httplib::detail::params_to_query_str(params);
    std::string url = makeCryptoUrl("/quotes?" + query_string, feed);

    httplib::SSLClient client(environment_.getDataHost());
    httplib::Result resp = client.Get(url.c_str(), makeHeaders(environment_));
    if (!resp) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an empty response";
        return std::make_pair(Status(1, ss.str()), crypto_quotes);
    }

    if (resp->status != 200) {
        std::ostringstream ss;
        ss << "Call to " << url << " returned an HTTP " << resp->status << ": " << resp->body;
        return std::make_pair(Status(1, ss.str()), crypto_quotes);
    }

    return std::make_pair(crypto_quotes.fromJSON(resp->body), crypto_quotes);
}

}  // namespace alpaca::markets
