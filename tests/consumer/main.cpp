#include <alpaca/markets/markets.hpp>
int main() {
    alpaca::markets::Trade trade;
    if (!trade.fromJSON(R"({"i":18446744073709551615,"s":0.5})").ok())
        return 1;
    return trade.id == 18446744073709551615ULL && trade.size == 0.5 ? 0 : 1;
}
