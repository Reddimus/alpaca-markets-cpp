#include <alpaca/markets/news.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status News::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing news JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a news object");
    }

    PARSE_UINT64(id, "id")
    PARSE_STRING(headline, "headline")
    PARSE_STRING(author, "author")
    PARSE_STRING(created_at, "created_at")
    PARSE_STRING(updated_at, "updated_at")
    PARSE_STRING(summary, "summary")
    PARSE_STRING(content, "content")
    PARSE_STRING(url, "url")
    PARSE_STRING(source, "source")
    PARSE_VECTOR_STRINGS(symbols, "symbols")

    // Parse images array
    if (d.HasMember("images") && d["images"].IsArray()) {
        for (auto& item : d["images"].GetArray()) {
            if (item.IsObject()) {
                NewsImage img;
                if (item.HasMember("size") && item["size"].IsString()) {
                    img.size = item["size"].GetString();
                }
                if (item.HasMember("url") && item["url"].IsString()) {
                    img.url = item["url"].GetString();
                }
                images.push_back(img);
            }
        }
    }

    return Status();
}

Status NewsArticles::fromJSON(const std::string& json) {
    rapidjson::Document d;
    if (d.Parse(json.c_str()).HasParseError()) {
        return Status(1, "Received parse error when deserializing news articles JSON");
    }

    if (!d.IsObject()) {
        return Status(1, "Deserialized valid JSON but it wasn't a news articles object");
    }

    // Parse news array
    if (d.HasMember("news") && d["news"].IsArray()) {
        for (auto& item : d["news"].GetArray()) {
            News article;
            rapidjson::StringBuffer s;
            s.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            item.Accept(writer);
            if (Status status = article.fromJSON(s.GetString()); !status.ok()) {
                return status;
            }
            news.push_back(article);
        }
    }

    PARSE_STRING(next_page_token, "next_page_token")

    return Status();
}

}  // namespace alpaca::markets
