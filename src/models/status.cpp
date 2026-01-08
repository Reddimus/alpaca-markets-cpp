#include <alpaca/markets/status.hpp>

namespace alpaca::markets {

std::string actionStatusToString(ActionStatus status) {
    switch (status) {
        case ActionStatus::Open:
            return "open";
        case ActionStatus::Closed:
            return "closed";
        case ActionStatus::Active:
            return "active";
        case ActionStatus::All:
            return "all";
        default:
            return "all";
    }
}

std::ostream& operator<<(std::ostream& os, const Status& s) {
    os << "Status(" << s.getCode() << ", \"" << s.getMessage() << "\")";
    return os;
}

}  // namespace alpaca::markets
