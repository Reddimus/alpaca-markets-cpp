#pragma once

// Shared complete JSON reads and checked numeric conversion for model parsers.

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace alpaca::markets::json_detail {
struct ReadOptions : glz::opts {
    bool validate_trailing_whitespace = true;
};
inline constexpr ReadOptions kReadOptions{{.null_terminated = false}};

template <class Integer>
bool exact_integer(const glz::generic_u64& value, Integer& out) {
    if (const std::uint64_t* number = value.get_if<std::uint64_t>()) {
        if (*number > static_cast<std::uint64_t>(std::numeric_limits<Integer>::max())) {
            return false;
        }
        out = static_cast<Integer>(*number);
        return true;
    }
    if (const std::int64_t* number = value.get_if<std::int64_t>()) {
        if constexpr (std::is_unsigned_v<Integer>) {
            if (*number < 0 || static_cast<std::uint64_t>(*number) > std::numeric_limits<Integer>::max()) {
                return false;
            }
        } else if (*number < std::numeric_limits<Integer>::min() || *number > std::numeric_limits<Integer>::max()) {
            return false;
        }
        out = static_cast<Integer>(*number);
        return true;
    }
    if (const double* number = value.get_if<double>()) {
        const double upper = std::ldexp(1.0, std::numeric_limits<Integer>::digits);
        const double lower = std::is_unsigned_v<Integer> ? 0.0 : -upper;
        if (!std::isfinite(*number) || *number < lower || *number >= upper || std::trunc(*number) != *number) {
            return false;
        }
        out = static_cast<Integer>(*number);
        return true;
    }
    return false;
}

// --- Type-predicate helpers ---------------------------------------------
// glz::generic_u64 exposes value-category queries via `is_*()`. We wrap them
// here so the macros below stay terse and don't have to chain `.find()` +
// `.is_*()` inline.

inline bool obj_get_string(const glz::generic_u64::object_t& obj, std::string_view key, std::string& out) {
    glz::generic_u64::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_string()) {
        return false;
    }
    out = it->second.get<std::string>();
    return true;
}

inline bool obj_get_bool(const glz::generic_u64::object_t& obj, std::string_view key, bool& out) {
    glz::generic_u64::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_boolean()) {
        return false;
    }
    out = it->second.get<bool>();
    return true;
}

inline bool obj_get_int(const glz::generic_u64::object_t& obj, std::string_view key, int& out) {
    const auto it = obj.find(key);
    return it != obj.end() && exact_integer(it->second, out);
}

inline bool obj_get_uint(const glz::generic_u64::object_t& obj, std::string_view key, unsigned int& out) {
    const auto it = obj.find(key);
    return it != obj.end() && exact_integer(it->second, out);
}

inline bool obj_get_uint64(const glz::generic_u64::object_t& obj, std::string_view key, std::uint64_t& out) {
    const auto it = obj.find(key);
    return it != obj.end() && exact_integer(it->second, out);
}

inline bool obj_get_double(const glz::generic_u64::object_t& obj, std::string_view key, double& out) {
    glz::generic_u64::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    const double value = it->second.as_number();
    if (!std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

inline bool obj_get_float(const glz::generic_u64::object_t& obj, std::string_view key, float& out) {
    glz::generic_u64::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    const double value = it->second.as_number();
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max()) {
        return false;
    }
    out = static_cast<float>(value);
    return true;
}

inline bool obj_get_vector_doubles(const glz::generic_u64::object_t& obj, std::string_view key,
                                   std::vector<double>& out) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    std::vector<double> values;
    values.reserve(it->second.get_array().size());
    for (const glz::generic_u64& element : it->second.get_array()) {
        if (element.is_null()) {
            values.push_back(std::numeric_limits<double>::quiet_NaN());
        } else if (element.is_number() && std::isfinite(element.as_number())) {
            values.push_back(element.as_number());
        } else {
            return false;
        }
    }
    out = std::move(values);
    return true;
}

inline bool obj_get_vector_uint64(const glz::generic_u64::object_t& obj, std::string_view key,
                                  std::vector<std::uint64_t>& out) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    std::vector<std::uint64_t> values;
    values.reserve(it->second.get_array().size());
    for (const glz::generic_u64& element : it->second.get_array()) {
        std::uint64_t value = 0;
        if (!exact_integer(element, value)) {
            return false;
        }
        values.push_back(value);
    }
    out = std::move(values);
    return true;
}

inline bool obj_get_vector_strings(const glz::generic_u64::object_t& obj, std::string_view key,
                                   std::vector<std::string>& out) {
    glz::generic_u64::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    const glz::generic_u64::array_t& arr = it->second.get_array();
    std::vector<std::string> items;
    items.reserve(arr.size());
    for (const glz::generic_u64& elem : arr) {
        if (elem.is_string()) {
            items.push_back(elem.get<std::string>());
        }
    }
    out = std::move(items);
    return true;
}

// Serialize an arbitrary glz::generic_u64 AST node back to a JSON string. Used by
// the model dispatchers (`Snapshot`, `Bars`-by-symbol, etc.) that hand a
// nested object's JSON text to a nested model's fromJSON(). Mirrors the
// RapidJSON `StringBuffer + Writer + node.Accept(writer)` pattern.
inline std::string write(const glz::generic_u64& node) {
    std::string out;
    (void)glz::write_json(node, out);
    return out;
}

}  // namespace alpaca::markets::json_detail

// PARSE_* macros - replacements for the RapidJSON-era helpers in this same
// file. Each macro takes `node` (a `const glz::generic_u64::object_t&` named
// exactly `node`) implicitly. The fromJSON() prelude that constructs `node`
// is provided by JSON_FROMJSON_PRELUDE below.

#define PARSE_STRING(var, name) ::alpaca::markets::json_detail::obj_get_string(node, (name), (var))

#define PARSE_INT(var, name)                                                     \
    do {                                                                         \
        const auto field = node.find(name);                                      \
        if (field != node.end() && !field->second.is_null() &&                   \
            !::alpaca::markets::json_detail::obj_get_int(node, (name), (var))) { \
            return Status(1, "Invalid integer field: " name);                    \
        }                                                                        \
    } while (false)

#define PARSE_UINT(var, name)                                                     \
    do {                                                                          \
        const auto field = node.find(name);                                       \
        if (field != node.end() && !field->second.is_null() &&                    \
            !::alpaca::markets::json_detail::obj_get_uint(node, (name), (var))) { \
            return Status(1, "Invalid integer field: " name);                     \
        }                                                                         \
    } while (false)

#define PARSE_UINT64(var, name)                                                     \
    do {                                                                            \
        const auto field = node.find(name);                                         \
        if (field != node.end() && !field->second.is_null() &&                      \
            !::alpaca::markets::json_detail::obj_get_uint64(node, (name), (var))) { \
            return Status(1, "Invalid integer field: " name);                       \
        }                                                                           \
    } while (false)

#define PARSE_BOOL(var, name) ::alpaca::markets::json_detail::obj_get_bool(node, (name), (var))

#define PARSE_DOUBLE(var, name)                                                     \
    do {                                                                            \
        const auto field = node.find(name);                                         \
        if (field != node.end() && !field->second.is_null() &&                      \
            !::alpaca::markets::json_detail::obj_get_double(node, (name), (var))) { \
            return Status(1, "Invalid numeric field: " name);                       \
        }                                                                           \
    } while (false)

#define PARSE_FLOAT(var, name)                                                     \
    do {                                                                           \
        const auto field = node.find(name);                                        \
        if (field != node.end() && !field->second.is_null() &&                     \
            !::alpaca::markets::json_detail::obj_get_float(node, (name), (var))) { \
            return Status(1, "Invalid numeric field: " name);                      \
        }                                                                          \
    } while (false)

#define PARSE_VECTOR_DOUBLES(var, name)                                                     \
    do {                                                                                    \
        const auto field = node.find(name);                                                 \
        if (field != node.end() && !field->second.is_null() &&                              \
            !::alpaca::markets::json_detail::obj_get_vector_doubles(node, (name), (var))) { \
            return Status(1, "Invalid numeric field: " name);                               \
        }                                                                                   \
    } while (false)

#define PARSE_VECTOR_UINT64(var, name)                                                     \
    do {                                                                                   \
        const auto field = node.find(name);                                                \
        if (field != node.end() && !field->second.is_null() &&                             \
            !::alpaca::markets::json_detail::obj_get_vector_uint64(node, (name), (var))) { \
            return Status(1, "Invalid integer field: " name);                              \
        }                                                                                  \
    } while (false)

#define PARSE_VECTOR_STRINGS(var, name) ::alpaca::markets::json_detail::obj_get_vector_strings(node, (name), (var))

// Standard prelude for fromJSON() methods. Parses the body into a
// glz::generic_u64 root, returns a parse-error Status if invalid or non-object,
// and binds `node` to the root's object map. Caller then uses PARSE_*.
//
// Usage:
//
//   Status Foo::fromJSON(const std::string& json) {
//       JSON_FROMJSON_PRELUDE("foo");
//       PARSE_STRING(name, "name");
//       ...
//       return Status();
//   }
//
// The TAG argument is used in the error message so it matches the
// pre-migration wording ("Received parse error when deserializing <tag> JSON").
// Only one prelude per function body (uses a fixed-name root).
#define JSON_FROMJSON_PRELUDE(TAG)                                                                           \
    ::glz::generic_u64 _glaze_root{};                                                                        \
    {                                                                                                        \
        ::glz::error_ctx _ec = ::glz::read<::alpaca::markets::json_detail::kReadOptions>(_glaze_root, json); \
        if (_ec) {                                                                                           \
            return Status(1, "Received parse error when deserializing " TAG " JSON");                        \
        }                                                                                                    \
    }                                                                                                        \
    if (!_glaze_root.is_object()) {                                                                          \
        return Status(1, "Deserialized valid JSON but it wasn't a " TAG " object");                          \
    }                                                                                                        \
    *this = std::remove_cvref_t<decltype(*this)>{};                                                          \
    const ::glz::generic_u64::object_t& node = _glaze_root.get_object()
