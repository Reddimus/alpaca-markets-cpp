#include <alpaca/markets/news.hpp>

#include "../detail/json.hpp"

namespace alpaca::markets {

Status News::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("news");

    PARSE_UINT64(id, "id");
    PARSE_STRING(headline, "headline");
    PARSE_STRING(author, "author");
    PARSE_STRING(created_at, "created_at");
    PARSE_STRING(updated_at, "updated_at");
    PARSE_STRING(summary, "summary");
    PARSE_STRING(content, "content");
    PARSE_STRING(url, "url");
    PARSE_STRING(source, "source");
    PARSE_VECTOR_STRINGS(symbols, "symbols");

    // Parse images array
    glz::generic::object_t::const_iterator images_it = node.find("images");
    if (images_it != node.end() && images_it->second.is_array()) {
        for (const glz::generic& item : images_it->second.get_array()) {
            if (!item.is_object()) {
                continue;
            }
            const glz::generic::object_t& img_obj = item.get_object();
            NewsImage img;
            json_detail::obj_get_string(img_obj, "size", img.size);
            json_detail::obj_get_string(img_obj, "url", img.url);
            images.push_back(img);
        }
    }

    return Status();
}

Status NewsArticles::fromJSON(const std::string& json) {
    JSON_FROMJSON_PRELUDE("news articles");

    // Parse news array
    glz::generic::object_t::const_iterator news_it = node.find("news");
    if (news_it != node.end() && news_it->second.is_array()) {
        for (const glz::generic& item : news_it->second.get_array()) {
            News article;
            if (Status status = article.fromJSON(json_detail::write(item)); !status.ok()) {
                return status;
            }
            news.push_back(article);
        }
    }

    PARSE_STRING(next_page_token, "next_page_token");

    return Status();
}

}  // namespace alpaca::markets
