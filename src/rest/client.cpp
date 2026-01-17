#include <alpaca/markets/client.hpp>

#include <httplib.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <iostream>
#include <map>
#include <sstream>
#include <utility>

namespace alpaca::markets {

namespace {
const char* kJSONContentType = "application/json";

httplib::Headers makeHeaders(const Environment& environment) {
    return {
        {"APCA-API-KEY-ID", environment.getAPIKeyID()},
        {"APCA-API-SECRET-KEY", environment.getAPISecretKey()},
    };
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
    
    rapidjson::Document d;
    if (!d.Parse(body.c_str()).HasParseError() && d.IsObject()) {
        if (d.HasMember("code") && d["code"].IsInt()) {
            api_code = d["code"].GetInt();
        }
        if (d.HasMember("message") && d["message"].IsString()) {
            message = d["message"].GetString();
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

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("no_shorting");
    writer.Bool(no_shorting);

    writer.Key("dtbp_check");
    writer.String(dtbp_check.c_str());

    writer.Key("trade_confirm_email");
    writer.String(trade_confirm_email.c_str());

    writer.Key("suspend_trade");
    writer.Bool(suspend_trade);

    writer.EndObject();
    const char* body = s.GetString();

    httplib::SSLClient client(environment_.getTradingHost());
    httplib::Result resp = client.Patch("/v2/account/configurations", makeHeaders(environment_), body, kJSONContentType);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing activities JSON"), activities);
    }
    for (auto& a : d.GetArray()) {
        std::string activity_type;
        if (a.HasMember("activity_type") && a["activity_type"].IsString()) {
            activity_type = a["activity_type"].GetString();
        } else {
            return std::make_pair(Status(1, "Activity didn't have activity_type attribute"), activities);
        }

        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        a.Accept(writer);

        if (activity_type == "FILL") {
            TradeActivity activity;
            if (Status status = activity.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, activities);
            }
            activities.push_back(activity);
        } else {
            NonTradeActivity activity;
            if (Status status = activity.fromJSON(s.GetString()); !status.ok()) {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing orders JSON"), orders);
    }
    for (auto& o : d.GetArray()) {
        Order order;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status parse_status = order.fromJSON(s.GetString()); !parse_status.ok()) {
            return std::make_pair(parse_status, orders);
        }
        orders.push_back(order);
    }

    return std::make_pair(Status(), orders);
}

std::pair<Status, Order> Client::submitOrder(const std::string& symbol, int quantity, OrderSide side, OrderType type,
                                             OrderTimeInForce tif, const std::string& limit_price,
                                             const std::string& stop_price, bool extended_hours,
                                             const std::string& client_order_id, OrderClass order_class,
                                             TakeProfitParams* take_profit_params,
                                             StopLossParams* stop_loss_params,
                                             const std::string& trail_price,
                                             const std::string& trail_percent) const {
    Order order;

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("symbol");
    writer.String(symbol.c_str());

    writer.Key("qty");
    writer.Int(quantity);

    writer.Key("side");
    writer.String(orderSideToString(side).c_str());

    writer.Key("type");
    writer.String(orderTypeToString(type).c_str());

    writer.Key("time_in_force");
    writer.String(orderTimeInForceToString(tif).c_str());

    if (!limit_price.empty()) {
        writer.Key("limit_price");
        writer.String(limit_price.c_str());
    }

    if (!stop_price.empty()) {
        writer.Key("stop_price");
        writer.String(stop_price.c_str());
    }

    // Trailing stop parameters
    if (!trail_price.empty()) {
        writer.Key("trail_price");
        writer.String(trail_price.c_str());
    }

    if (!trail_percent.empty()) {
        writer.Key("trail_percent");
        writer.String(trail_percent.c_str());
    }

    if (extended_hours) {
        writer.Key("extended_hours");
        writer.Bool(extended_hours);
    }

    if (!client_order_id.empty()) {
        writer.Key("client_order_id");
        writer.String(client_order_id.c_str());
    }

    if (order_class != OrderClass::Simple) {
        writer.Key("order_class");
        writer.String(orderClassToString(order_class).c_str());
    }

    if (take_profit_params != nullptr) {
        writer.Key("take_profit");
        writer.StartObject();
        if (!take_profit_params->limitPrice.empty()) {
            writer.Key("limit_price");
            writer.String(take_profit_params->limitPrice.c_str());
        }
        writer.EndObject();
    }

    if (stop_loss_params != nullptr) {
        writer.Key("stop_loss");
        writer.StartObject();
        if (!stop_loss_params->limitPrice.empty()) {
            writer.Key("limit_price");
            writer.String(stop_loss_params->limitPrice.c_str());
        }
        if (!stop_loss_params->stopPrice.empty()) {
            writer.Key("stop_price");
            writer.String(stop_loss_params->stopPrice.c_str());
        }
        writer.EndObject();
    }

    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("symbol");
    writer.String(symbol.c_str());

    writer.Key("notional");
    writer.String(notional.c_str());

    writer.Key("side");
    writer.String(orderSideToString(side).c_str());

    writer.Key("type");
    writer.String(orderTypeToString(type).c_str());

    writer.Key("time_in_force");
    writer.String(orderTimeInForceToString(tif).c_str());

    if (!limit_price.empty()) {
        writer.Key("limit_price");
        writer.String(limit_price.c_str());
    }

    if (extended_hours) {
        writer.Key("extended_hours");
        writer.Bool(extended_hours);
    }

    if (!client_order_id.empty()) {
        writer.Key("client_order_id");
        writer.String(client_order_id.c_str());
    }

    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("qty");
    writer.Int(quantity);

    writer.Key("time_in_force");
    writer.String(orderTimeInForceToString(tif).c_str());

    if (!limit_price.empty()) {
        writer.Key("limit_price");
        writer.String(limit_price.c_str());
    }

    if (!stop_price.empty()) {
        writer.Key("stop_price");
        writer.String(stop_price.c_str());
    }

    if (!client_order_id.empty()) {
        writer.Key("client_order_id");
        writer.String(client_order_id.c_str());
    }

    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing orders JSON"), orders);
    }
    for (auto& o : d.GetArray()) {
        Order order;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = order.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, orders);
        }
        orders.push_back(order);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing positions JSON"), positions);
    }
    for (auto& o : d.GetArray()) {
        Position position;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = position.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, positions);
        }
        positions.push_back(position);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing positions JSON"), positions);
    }
    for (auto& o : d.GetArray()) {
        Position position;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = position.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, positions);
        }
        positions.push_back(position);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing assets JSON"), assets);
    }
    for (auto& o : d.GetArray()) {
        Asset asset;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = asset.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, assets);
        }
        assets.push_back(asset);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing calendar JSON"), dates);
    }
    for (auto& o : d.GetArray()) {
        Date date;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = date.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, dates);
        }
        dates.push_back(date);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing watchlists JSON"), watchlists);
    }
    for (auto& o : d.GetArray()) {
        Watchlist watchlist;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = watchlist.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, watchlists);
        }
        watchlists.push_back(watchlist);
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

std::pair<Status, Watchlist> Client::createWatchlist(const std::string& name,
                                                     const std::vector<std::string>& symbols) const {
    Watchlist watchlist;

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("name");
    writer.String(name.c_str());

    writer.Key("symbols");
    writer.StartArray();
    for (const auto& symbol : symbols) {
        writer.String(symbol.c_str());
    }
    writer.EndArray();

    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();

    writer.Key("name");
    writer.String(name.c_str());

    writer.Key("symbols");
    writer.StartArray();
    for (const auto& symbol : symbols) {
        writer.String(symbol.c_str());
    }
    writer.EndArray();

    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::StringBuffer s;
    s.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("symbol");
    writer.String(symbol.c_str());
    writer.EndObject();
    const char* body = s.GetString();

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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest trades JSON"), trades);
    }

    if (d.HasMember("trades") && d["trades"].IsObject()) {
        for (auto& m : d["trades"].GetObject()) {
            Trade trade;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, trades);
            }
            trades[m.name.GetString()] = trade;
        }
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest quotes JSON"), quotes);
    }

    if (d.HasMember("quotes") && d["quotes"].IsObject()) {
        for (auto& m : d["quotes"].GetObject()) {
            Quote quote;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = quote.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, quotes);
            }
            quotes[m.name.GetString()] = quote;
        }
    }

    return std::make_pair(Status(), quotes);
}

// ==================== Corporate Actions ====================

std::pair<Status, std::vector<Announcement>> Client::getAnnouncements(
    const std::vector<std::string>& ca_types,
    const std::string& since,
    const std::string& until,
    const std::string& symbol,
    const std::string& cusip,
    const std::string& date_type) const {
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
        if (!query_string.empty()) query_string += "&";
        query_string += "since=" + since;
    }

    if (!until.empty()) {
        if (!query_string.empty()) query_string += "&";
        query_string += "until=" + until;
    }

    if (!symbol.empty()) {
        if (!query_string.empty()) query_string += "&";
        query_string += "symbol=" + symbol;
    }

    if (!cusip.empty()) {
        if (!query_string.empty()) query_string += "&";
        query_string += "cusip=" + cusip;
    }

    if (!date_type.empty()) {
        if (!query_string.empty()) query_string += "&";
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing announcements JSON"), announcements);
    }

    if (!d.IsArray()) {
        return std::make_pair(Status(1, "Expected array of announcements"), announcements);
    }

    for (auto& o : d.GetArray()) {
        Announcement announcement;
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        o.Accept(writer);
        if (Status status = announcement.fromJSON(s.GetString()); !status.ok()) {
            return std::make_pair(status, announcements);
        }
        announcements.push_back(announcement);
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
    const std::string& underlying_symbols,
    const std::string& status,
    const std::string& expiration_date,
    const std::string& expiration_date_gte,
    const std::string& expiration_date_lte,
    const std::string& root_symbol,
    const std::string& type,
    const std::string& style,
    const std::string& strike_price_gte,
    const std::string& strike_price_lte,
    unsigned int limit,
    const std::string& page_token) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing snapshots JSON"), snapshots);
    }

    // Response is object keyed by symbol
    if (d.IsObject()) {
        for (auto& m : d.GetObject()) {
            Snapshot snapshot;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = snapshot.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, snapshots);
            }
            snapshots[m.name.GetString()] = snapshot;
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest bar JSON"), bar);
    }

    if (d.HasMember("bar") && d["bar"].IsObject()) {
        rapidjson::StringBuffer s;
        s.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        d["bar"].Accept(writer);
        return std::make_pair(bar.fromJSON(s.GetString()), bar);
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing latest bars JSON"), bars);
    }

    if (d.HasMember("bars") && d["bars"].IsObject()) {
        for (auto& m : d["bars"].GetObject()) {
            Bar bar;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = bar.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, bars);
            }
            bars[m.name.GetString()] = bar;
        }
    }

    return std::make_pair(Status(), bars);
}

// ==================== Market Data - Historical Trades/Quotes ====================

std::pair<Status, std::pair<std::vector<Trade>, std::string>> Client::getTrades(
    const std::string& symbol,
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing trades JSON"), 
                              std::make_pair(trades, next_page_token));
    }

    if (d.HasMember("trades") && d["trades"].IsArray()) {
        for (auto& o : d["trades"].GetArray()) {
            Trade trade;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            o.Accept(writer);
            if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, std::make_pair(trades, next_page_token));
            }
            trades.push_back(trade);
        }
    }

    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return std::make_pair(Status(), std::make_pair(trades, next_page_token));
}

std::pair<Status, std::pair<std::vector<Quote>, std::string>> Client::getQuotes(
    const std::string& symbol,
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing quotes JSON"),
                              std::make_pair(quotes, next_page_token));
    }

    if (d.HasMember("quotes") && d["quotes"].IsArray()) {
        for (auto& o : d["quotes"].GetArray()) {
            Quote quote;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            o.Accept(writer);
            if (Status status = quote.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, std::make_pair(quotes, next_page_token));
            }
            quotes.push_back(quote);
        }
    }

    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return std::make_pair(Status(), std::make_pair(quotes, next_page_token));
}

// ==================== Market Data - Multi-Symbol Historical ====================

std::pair<Status, MultiTrades> Client::getMultiTrades(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
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

std::pair<Status, MultiQuotes> Client::getMultiQuotes(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
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

std::pair<Status, Auctions> Client::getAuctions(
    const std::string& symbol,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
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

std::pair<Status, Auctions> Client::getMultiAuctions(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
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

std::pair<Status, CorporateActions> Client::getCorporateActions(
    const std::vector<std::string>& symbols,
    const std::vector<std::string>& types,
    const std::string& start,
    const std::string& end,
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

std::pair<Status, NewsArticles> Client::getNews(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
    const std::string& page_token,
    bool include_content,
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
}  // namespace

std::pair<Status, CryptoTrade> Client::getLatestCryptoTrade(
    const std::string& symbol,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto trade JSON"), trade);
    }

    if (d.HasMember("trades") && d["trades"].IsObject()) {
        auto& trades_obj = d["trades"];
        if (trades_obj.HasMember(symbol.c_str()) && trades_obj[symbol.c_str()].IsObject()) {
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            trades_obj[symbol.c_str()].Accept(writer);
            return std::make_pair(trade.fromJSON(s.GetString()), trade);
        }
    }

    return std::make_pair(Status(1, "Trade not found for symbol"), trade);
}

std::pair<Status, std::map<std::string, CryptoTrade>> Client::getLatestCryptoTrades(
    const std::vector<std::string>& symbols,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto trades JSON"), trades);
    }

    if (d.HasMember("trades") && d["trades"].IsObject()) {
        for (auto& m : d["trades"].GetObject()) {
            CryptoTrade trade;
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = trade.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, trades);
            }
            trades[m.name.GetString()] = trade;
        }
    }

    return std::make_pair(Status(), trades);
}

std::pair<Status, CryptoQuote> Client::getLatestCryptoQuote(
    const std::string& symbol,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto quote JSON"), quote);
    }

    if (d.HasMember("quotes") && d["quotes"].IsObject()) {
        auto& quotes_obj = d["quotes"];
        if (quotes_obj.HasMember(symbol.c_str()) && quotes_obj[symbol.c_str()].IsObject()) {
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            quotes_obj[symbol.c_str()].Accept(writer);
            return std::make_pair(quote.fromJSON(s.GetString()), quote);
        }
    }

    return std::make_pair(Status(1, "Quote not found for symbol"), quote);
}

std::pair<Status, std::map<std::string, CryptoQuote>> Client::getLatestCryptoQuotes(
    const std::vector<std::string>& symbols,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto quotes JSON"), quotes);
    }

    if (d.HasMember("quotes") && d["quotes"].IsObject()) {
        for (auto& m : d["quotes"].GetObject()) {
            CryptoQuote quote;
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = quote.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, quotes);
            }
            quotes[m.name.GetString()] = quote;
        }
    }

    return std::make_pair(Status(), quotes);
}

std::pair<Status, CryptoBar> Client::getLatestCryptoBar(
    const std::string& symbol,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto bar JSON"), bar);
    }

    if (d.HasMember("bars") && d["bars"].IsObject()) {
        auto& bars_obj = d["bars"];
        if (bars_obj.HasMember(symbol.c_str()) && bars_obj[symbol.c_str()].IsObject()) {
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            bars_obj[symbol.c_str()].Accept(writer);
            return std::make_pair(bar.fromJSON(s.GetString()), bar);
        }
    }

    return std::make_pair(Status(1, "Bar not found for symbol"), bar);
}

std::pair<Status, std::map<std::string, CryptoBar>> Client::getLatestCryptoBars(
    const std::vector<std::string>& symbols,
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto bars JSON"), bars);
    }

    if (d.HasMember("bars") && d["bars"].IsObject()) {
        for (auto& m : d["bars"].GetObject()) {
            CryptoBar bar;
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = bar.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, bars);
            }
            bars[m.name.GetString()] = bar;
        }
    }

    return std::make_pair(Status(), bars);
}

std::pair<Status, CryptoSnapshot> Client::getCryptoSnapshot(
    const std::string& symbol,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto snapshot JSON"), snapshot);
    }

    if (d.HasMember("snapshots") && d["snapshots"].IsObject()) {
        auto& snapshots_obj = d["snapshots"];
        if (snapshots_obj.HasMember(symbol.c_str()) && snapshots_obj[symbol.c_str()].IsObject()) {
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            snapshots_obj[symbol.c_str()].Accept(writer);
            return std::make_pair(snapshot.fromJSON(s.GetString()), snapshot);
        }
    }

    return std::make_pair(Status(1, "Snapshot not found for symbol"), snapshot);
}

std::pair<Status, std::map<std::string, CryptoSnapshot>> Client::getCryptoSnapshots(
    const std::vector<std::string>& symbols,
    CryptoFeed feed) const {
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

    rapidjson::Document d;
    if (d.Parse(resp->body.c_str()).HasParseError()) {
        return std::make_pair(Status(1, "Received parse error when deserializing crypto snapshots JSON"), snapshots);
    }

    if (d.HasMember("snapshots") && d["snapshots"].IsObject()) {
        for (auto& m : d["snapshots"].GetObject()) {
            CryptoSnapshot snapshot;
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            m.value.Accept(writer);
            if (Status status = snapshot.fromJSON(s.GetString()); !status.ok()) {
                return std::make_pair(status, snapshots);
            }
            snapshots[m.name.GetString()] = snapshot;
        }
    }

    return std::make_pair(Status(), snapshots);
}

std::pair<Status, CryptoBars> Client::getCryptoBars(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    const std::string& timeframe,
    unsigned int limit,
    const std::string& page_token,
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

std::pair<Status, CryptoTrades> Client::getCryptoTrades(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
    const std::string& page_token,
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

std::pair<Status, CryptoQuotes> Client::getCryptoQuotes(
    const std::vector<std::string>& symbols,
    const std::string& start,
    const std::string& end,
    unsigned int limit,
    const std::string& page_token,
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
