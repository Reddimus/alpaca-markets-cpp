#pragma once

#include <ostream>
#include <string>
#include <utility>

namespace alpaca::markets {

/**
 * @brief The status of various Alpaca actions.
 */
enum class ActionStatus {
    Open,
    Closed,
    Active,
    All,
};

/**
 * @brief A helper to convert an ActionStatus to a string
 */
std::string actionStatusToString(ActionStatus status);

/**
 * @brief A utility class which is used to express the state of operations.
 *
 * @code{.cpp}
 *   alpaca::markets::Status foobar() {
 *     SomeType na = doSomeWork();
 *     if (na->itWorked()) {
 *       return alpaca::markets::Status(0, "OK");
 *     } else {
 *       return alpaca::markets::Status(1, na->getErrorString());
 *     }
 *   }
 * @endcode
 */
class Status {
public:
    /**
     * @brief Default constructor
     *
     * Note that the default constructor initialized an alpaca::markets::Status instance
     * to a state such that a successful operation is indicated.
     */
    explicit Status(int c = 0) : code_(c), message_("OK") {}

    /**
     * @brief A constructor which can be used to concisely express the status of
     * an operation.
     *
     * @param c a status code. The idiom is that a zero status code indicates a
     * successful operation and a non-zero status code indicates a failed
     * operation.
     * @param m a message indicating some extra detail regarding the operation.
     * If all operations were successful, this message should be "OK".
     * Otherwise, it doesn't matter what the string is, as long as both the
     * setter and caller agree.
     */
    Status(int c, std::string m) : code_(c), message_(std::move(m)) {}

public:
    /**
     * @brief A getter for the status code property
     *
     * @return an integer representing the status code of the operation.
     */
    [[nodiscard]] int getCode() const { return code_; }

    /**
     * @brief A getter for the message property
     *
     * @return a string representing arbitrary additional information about the
     * success or failure of an operation. On successful operations, the idiom
     * is for the message to be "OK"
     */
    [[nodiscard]] std::string getMessage() const { return message_; }

    /**
     * @brief A convenience method to check if the return code is 0
     *
     * @code{.cpp}
     *   alpaca::markets::Status s = doSomething();
     *   if (s.ok()) {
     *     LOG(INFO) << "doing work";
     *   } else {
     *     LOG(ERROR) << s.toString();
     *   }
     * @endcode
     *
     * @return a boolean which is true if the status code is 0, false otherwise.
     */
    [[nodiscard]] bool ok() const { return getCode() == 0; }

    /**
     * @brief A synonym for alpaca::markets::Status::getMessage()
     *
     * @see getMessage()
     */
    [[nodiscard]] std::string toString() const { return getMessage(); }

    /**
     * @brief A synonym for alpaca::markets::Status::getMessage()
     *
     * @see getMessage()
     */
    [[nodiscard]] std::string what() const { return getMessage(); }

    /**
     * @brief implicit conversion to bool
     *
     * Allows easy use of Status in an if statement, as below:
     *
     * @code{.cpp}
     *   if (doSomethingThatReturnsStatus()) {
     *     LOG(INFO) << "Success!";
     *   }
     * @endcode
     */
    explicit operator bool() const { return ok(); }

    // Below operator implementations useful for testing with gtest

    // Enables use of gtest (ASSERT|EXPECT)_EQ
    bool operator==(const Status& rhs) const { return (code_ == rhs.getCode()) && (message_ == rhs.getMessage()); }

    // Enables use of gtest (ASSERT|EXPECT)_NE
    bool operator!=(const Status& rhs) const { return !operator==(rhs); }

    // Enables pretty-printing in gtest (ASSERT|EXPECT)_(EQ|NE)
    friend std::ostream& operator<<(std::ostream& os, const Status& s);

private:
    int code_;
    std::string message_;
};

/**
 * @brief API Error from Alpaca REST API responses.
 *
 * This class wraps the detailed error code and message supplied
 * by Alpaca's API for debugging purposes. Similar to the Go SDK's APIError.
 *
 * @code{.cpp}
 *   alpaca::markets::APIError err(422, 40010000, "insufficient qty available for order");
 *   std::cerr << err.what() << std::endl;
 *   // Output: insufficient qty available for order (HTTP 422, Code 40010000)
 * @endcode
 */
class APIError {
public:
    /**
     * @brief Construct an API error from HTTP response details
     *
     * @param http_status_code The HTTP status code from the response
     * @param api_code The Alpaca-specific error code from JSON body
     * @param message The error message from JSON body
     * @param body The raw response body (optional)
     */
    APIError(int http_status_code, int api_code, std::string message, std::string body = "")
        : http_status_code_(http_status_code),
          api_code_(api_code),
          message_(std::move(message)),
          body_(std::move(body)) {}

    /**
     * @brief Get the HTTP status code
     */
    [[nodiscard]] int getHTTPStatusCode() const { return http_status_code_; }

    /**
     * @brief Get the Alpaca API error code
     */
    [[nodiscard]] int getAPICode() const { return api_code_; }

    /**
     * @brief Get the error message
     */
    [[nodiscard]] const std::string& getMessage() const { return message_; }

    /**
     * @brief Get the raw response body
     */
    [[nodiscard]] const std::string& getBody() const { return body_; }

    /**
     * @brief Get formatted error string similar to Go SDK
     */
    [[nodiscard]] std::string what() const;

    /**
     * @brief Convert to Status for compatibility
     */
    [[nodiscard]] Status toStatus() const { return Status(1, what()); }

private:
    int http_status_code_;
    int api_code_;
    std::string message_;
    std::string body_;
};

}  // namespace alpaca::markets
