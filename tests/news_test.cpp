#include <alpaca/markets/news.hpp>

#include <gtest/gtest.h>

using namespace alpaca::markets;

TEST(NewsTest, FromJSON) {
    const std::string json = R"({
        "id": 12345678,
        "headline": "Apple Reports Record Q4 Earnings",
        "author": "John Smith",
        "created_at": "2024-01-10T18:30:00Z",
        "updated_at": "2024-01-10T18:35:00Z",
        "summary": "Apple Inc. reported record fourth-quarter earnings...",
        "content": "Full article content here...",
        "url": "https://example.com/news/apple-q4",
        "source": "bloomberg",
        "symbols": ["AAPL", "MSFT"],
        "images": [
            {"size": "large", "url": "https://example.com/img/large.jpg"},
            {"size": "thumb", "url": "https://example.com/img/thumb.jpg"}
        ]
    })";

    News news;
    Status status = news.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(news.id, 12345678u);
    EXPECT_EQ(news.headline, "Apple Reports Record Q4 Earnings");
    EXPECT_EQ(news.author, "John Smith");
    EXPECT_EQ(news.created_at, "2024-01-10T18:30:00Z");
    EXPECT_EQ(news.summary, "Apple Inc. reported record fourth-quarter earnings...");
    EXPECT_EQ(news.source, "bloomberg");
    EXPECT_EQ(news.symbols.size(), 2u);
    EXPECT_EQ(news.symbols[0], "AAPL");
    EXPECT_EQ(news.symbols[1], "MSFT");
    EXPECT_EQ(news.images.size(), 2u);
    EXPECT_EQ(news.images[0].size, "large");
    EXPECT_EQ(news.images[1].size, "thumb");
}

TEST(NewsArticlesTest, FromJSON) {
    const std::string json = R"({
        "news": [
            {
                "id": 1,
                "headline": "First Article",
                "author": "Author 1",
                "source": "source1",
                "symbols": ["AAPL"]
            },
            {
                "id": 2,
                "headline": "Second Article",
                "author": "Author 2",
                "source": "source2",
                "symbols": ["GOOG"]
            }
        ],
        "next_page_token": "nexttoken123"
    })";

    NewsArticles articles;
    Status status = articles.fromJSON(json);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(articles.news.size(), 2u);
    EXPECT_EQ(articles.news[0].headline, "First Article");
    EXPECT_EQ(articles.news[1].headline, "Second Article");
    EXPECT_EQ(articles.next_page_token, "nexttoken123");
}

TEST(NewsTest, FromJSONParseError) {
    News news;
    Status status = news.fromJSON("invalid json");
    EXPECT_FALSE(status.ok());
}
