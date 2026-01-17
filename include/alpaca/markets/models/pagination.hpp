#pragma once

#include <alpaca/markets/models/status.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace alpaca::markets {

/**
 * @brief A page of results from a paginated API endpoint.
 * 
 * @tparam T The type of items in the page
 */
template<typename T>
struct Page {
    std::vector<T> items;
    std::string next_page_token;
    
    /**
     * @brief Check if there are more pages available
     */
    [[nodiscard]] bool hasMore() const { return !next_page_token.empty(); }
};

/**
 * @brief Iterator for paginated API endpoints.
 * 
 * Provides lazy iteration over paginated results, fetching pages on demand.
 * 
 * Example usage:
 * @code{.cpp}
 *   auto [status, page] = client.getTrades("AAPL", start, end, 100);
 *   if (!status.ok()) return;
 *   
 *   // Process first page
 *   for (const auto& trade : page.first) {
 *       process(trade);
 *   }
 *   
 *   // Fetch more pages using the next_page_token
 *   while (!page.second.empty()) {
 *       auto [s, p] = client.getTrades("AAPL", start, end, 100, page.second);
 *       if (!s.ok()) break;
 *       page = p;
 *       for (const auto& trade : page.first) {
 *           process(trade);
 *       }
 *   }
 * @endcode
 * 
 * @tparam T The type of items being iterated
 */
template<typename T>
class PageIterator {
public:
    using FetchFunc = std::function<std::pair<Status, Page<T>>(const std::string& page_token)>;

    /**
     * @brief Construct a PageIterator with a fetch function.
     * 
     * @param fetch_func Function that fetches a page given a page token
     */
    explicit PageIterator(FetchFunc fetch_func) 
        : fetch_func_(std::move(fetch_func)), exhausted_(false) {}

    /**
     * @brief Get the next page of results.
     * 
     * @return Status and Page containing items and next_page_token
     */
    std::pair<Status, Page<T>> next() {
        if (exhausted_) {
            return std::make_pair(Status(1, "Iterator exhausted"), Page<T>{});
        }
        
        auto [status, page] = fetch_func_(current_page_token_);
        if (!status.ok()) {
            return std::make_pair(status, Page<T>{});
        }
        
        current_page_token_ = page.next_page_token;
        if (current_page_token_.empty()) {
            exhausted_ = true;
        }
        
        return std::make_pair(Status(), page);
    }

    /**
     * @brief Check if there are more pages to fetch.
     */
    [[nodiscard]] bool hasMore() const {
        return !exhausted_;
    }

    /**
     * @brief Collect all remaining items into a single vector.
     * 
     * Warning: This may use significant memory for large result sets.
     * 
     * @return Status and vector of all items
     */
    std::pair<Status, std::vector<T>> collectAll() {
        std::vector<T> all_items;
        
        while (hasMore()) {
            auto [status, page] = next();
            if (!status.ok()) {
                return std::make_pair(status, all_items);
            }
            all_items.insert(all_items.end(), page.items.begin(), page.items.end());
        }
        
        return std::make_pair(Status(), all_items);
    }

private:
    FetchFunc fetch_func_;
    std::string current_page_token_;
    bool exhausted_;
};

/**
 * @brief Helper to create a PageIterator for trades.
 * 
 * @param client The API client
 * @param symbol Stock symbol
 * @param start Start time
 * @param end End time
 * @param limit Items per page
 */
template<typename Client>
PageIterator<class Trade> makeTradesIterator(
    const Client& client,
    const std::string& symbol,
    const std::string& start,
    const std::string& end,
    unsigned int limit = 1000) {
    return PageIterator<Trade>([&client, symbol, start, end, limit](const std::string& page_token) {
        auto [status, result] = client.getTrades(symbol, start, end, limit, page_token);
        Page<Trade> page;
        page.items = result.first;
        page.next_page_token = result.second;
        return std::make_pair(status, page);
    });
}

/**
 * @brief Helper to create a PageIterator for quotes.
 */
template<typename Client>
PageIterator<class Quote> makeQuotesIterator(
    const Client& client,
    const std::string& symbol,
    const std::string& start,
    const std::string& end,
    unsigned int limit = 1000) {
    return PageIterator<Quote>([&client, symbol, start, end, limit](const std::string& page_token) {
        auto [status, result] = client.getQuotes(symbol, start, end, limit, page_token);
        Page<Quote> page;
        page.items = result.first;
        page.next_page_token = result.second;
        return std::make_pair(status, page);
    });
}

}  // namespace alpaca::markets
