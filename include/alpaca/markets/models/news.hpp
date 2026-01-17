#pragma once

#include <alpaca/markets/models/status.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace alpaca::markets {

/**
 * @brief Image associated with a news article
 */
struct NewsImage {
    std::string size;   // "large", "small", "thumb"
    std::string url;
};

/**
 * @brief A type representing a news article.
 */
class News {
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
    uint64_t id = 0;
    std::string headline;
    std::string author;
    std::string created_at;
    std::string updated_at;
    std::string summary;
    std::string content;
    std::string url;
    std::vector<NewsImage> images;
    std::vector<std::string> symbols;
    std::string source;
};

/**
 * @brief Response containing multiple news articles with pagination
 */
class NewsArticles {
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
    std::vector<News> news;
    std::string next_page_token;
};

}  // namespace alpaca::markets
