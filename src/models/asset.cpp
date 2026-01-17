#include <alpaca/markets/asset.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

std::string assetClassToString(AssetClass asset_class) {
    switch (asset_class) {
        case AssetClass::USEquity:
            return "us_equity";
        case AssetClass::Crypto:
            return "crypto";
        default:
            return "us_equity";
    }
}

Status Asset::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing asset JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't an asset object");
    }

    PARSE_STRING(asset_class, "class")
    PARSE_BOOL(easy_to_borrow, "easy_to_borrow")
    PARSE_STRING(exchange, "exchange")
    PARSE_STRING(id, "id")
    PARSE_BOOL(marginable, "marginable")
    PARSE_BOOL(shortable, "shortable")
    PARSE_STRING(status, "status")
    PARSE_STRING(symbol, "symbol")
    PARSE_BOOL(tradable, "tradable")
    PARSE_BOOL(fractionable, "fractionable")
    PARSE_STRING(name, "name")
    PARSE_UINT(maintenance_margin_requirement, "maintenance_margin_requirement")

    return Status();
}

}  // namespace alpaca::markets
