#include <alpaca/markets/markets.hpp>

#include <chrono>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    alpaca::markets::Environment environment;
    if (!environment.parse().ok())
        return 3;
    alpaca::markets::RetryConfig retry;
    retry.max_retries = 2;
    retry.initial_delay = std::chrono::milliseconds{0};
    environment.setRetryConfig(retry);
    alpaca::markets::TimeoutConfig timeout;
    timeout.read_timeout = std::chrono::seconds{1};
    environment.setTimeoutConfig(timeout);
    alpaca::markets::Client client(environment);
    const std::string mode = argv[1];
    if (mode == "retry")
        return client.getAccount().first.ok() ? 0 : 1;
    if (mode == "query") {
        if (!client.getOrderByClientOrderID("a&nested=true").first.ok())
            return 1;
        if (!client.getPosition("BTC/USD?x=1").first.ok())
            return 1;
        if (!client.getCalendar("2026-10-25", "2026-10-26").first.ok())
            return 1;
        return 0;
    }
    if (mode == "write")
        return client.cancelOrder("fixture").first.ok() ? 1 : 0;
    if (mode == "timeout") {
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        const bool ok = client.getAccount().first.ok();
        return !ok && std::chrono::steady_clock::now() - start < std::chrono::seconds{3} ? 0 : 1;
    }
    return 2;
}
