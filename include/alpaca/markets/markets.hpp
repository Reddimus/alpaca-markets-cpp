#pragma once

/**
 * @file markets.hpp
 * @brief Umbrella header for the Alpaca Markets C++ SDK
 * 
 * This header includes all public headers of the alpaca::markets library.
 * 
 * @code{.cpp}
 * #include <alpaca/markets/markets.hpp>
 * // or use individual headers:
 * // #include <alpaca/markets/client.hpp>
 * 
 * int main() {
 *     alpaca::markets::Environment env;
 *     if (alpaca::markets::Status status = env.parse(); !status.ok()) {
 *         std::cerr << "Error: " << status.getMessage() << std::endl;
 *         return 1;
 *     }
 *     
 *     alpaca::markets::Client client(env);
 *     // Use client...
 * }
 * @endcode
 */

#include <alpaca/markets/account.hpp>
#include <alpaca/markets/asset.hpp>
#include <alpaca/markets/bars.hpp>
#include <alpaca/markets/calendar.hpp>
#include <alpaca/markets/client.hpp>
#include <alpaca/markets/clock.hpp>
#include <alpaca/markets/config.hpp>
#include <alpaca/markets/order.hpp>
#include <alpaca/markets/portfolio.hpp>
#include <alpaca/markets/position.hpp>
#include <alpaca/markets/quote.hpp>
#include <alpaca/markets/status.hpp>
#include <alpaca/markets/streaming.hpp>
#include <alpaca/markets/trade.hpp>
#include <alpaca/markets/watchlist.hpp>
