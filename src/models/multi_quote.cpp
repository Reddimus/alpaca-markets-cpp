#include <alpaca/markets/multi_quote.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status MultiQuotes::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing multi quotes JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a multi quotes object");
    }

    // Parse quotes map
    if (d.HasMember("quotes") && d["quotes"].IsObject()) {
        for (auto& m : d["quotes"].GetObject()) {
            std::vector<Quote> symbol_quotes;
            if (m.value.IsArray()) {
                for (auto& o : m.value.GetArray()) {
                    Quote quote;
                    rapidjson::StringBuffer s;
                    s.Clear();
                    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
                    o.Accept(writer);
                    if (Status status = quote.fromJSON(s.GetString()); !status.ok()) {
                        return status;
                    }
                    symbol_quotes.push_back(quote);
                }
            }
            quotes[m.name.GetString()] = symbol_quotes;
        }
    }

    // Parse next_page_token
    if (d.HasMember("next_page_token") && d["next_page_token"].IsString()) {
        next_page_token = d["next_page_token"].GetString();
    }

    return Status();
}

}  // namespace alpaca::markets
