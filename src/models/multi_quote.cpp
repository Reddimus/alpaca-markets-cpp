#include <alpaca/markets/multi_quote.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status MultiQuotes::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("multi quotes");

    // Parse quotes map
    glz::generic::object_t::const_iterator quotes_it = node.find("quotes");
    if (quotes_it != node.end() && quotes_it->second.is_object()) {
        for (const auto& [symbol, value] : quotes_it->second.get_object()) {
            std::vector<Quote> symbol_quotes;
            if (value.is_array()) {
                for (const glz::generic& item : value.get_array()) {
                    Quote quote;
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

}  // namespace alpaca::markets
