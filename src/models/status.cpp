#include <alpaca/markets/status.hpp>

#include <sstream>

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

std::string APIError::what() const {
    std::ostringstream ss;
    ss << message_ << " (HTTP " << http_status_code_;
    if (api_code_ != 0) {
        ss << ", Code " << api_code_;
    }
    ss << ")";
    return ss.str();
}

}  // namespace alpaca::markets
