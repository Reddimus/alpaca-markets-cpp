#pragma once

// JSON parsing helpers for Alpaca model fromJSON() methods.
//
// Migrated 2026-05-11 from RapidJSON to Glaze v7.6.0 (see
// CMakeLists.txt FetchContent block). The public model API
//   Status T::fromJSON(const std::string& json);
// is unchanged. Internally we now parse the body once into a
// glz::generic and walk the AST. This matches the open-meteo-cpp
// migration's `glz::generic` approach for payloads where some
// keys are user-driven / dynamic (Bars-by-symbol map keys etc.).
//
// The PARSE_* macros expand to scoped helpers that look up a key
// in the active object node and assign into `var` if the type
// matches. They take an implicit `node` variable in scope (a
// `const glz::generic::object_t&`). The previous RapidJSON
// macros took an implicit `d` (a `rapidjson::Document`); the
// macro names stay the same and the spelling stays the same to
// minimise churn inside each fromJSON() implementation.

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace alpaca::markets::json_detail {

// --- Type-predicate helpers ---------------------------------------------
// glz::generic exposes value-category queries via `is_*()`. We wrap them
// here so the macros below stay terse and don't have to chain `.find()` +
// `.is_*()` inline.

inline bool obj_get_string(const glz::generic::object_t& obj, std::string_view key, std::string& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_string()) {
        return false;
    }
    out = it->second.get<std::string>();
    return true;
}

inline bool obj_get_bool(const glz::generic::object_t& obj, std::string_view key, bool& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_boolean()) {
        return false;
    }
    out = it->second.get<bool>();
    return true;
}

inline bool obj_get_int(const glz::generic::object_t& obj, std::string_view key, int& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    // Glaze stores numbers as double internally; cast safely for ints
    // within range. Anything outside int range hits implementation-defined
    // behaviour but won't crash; the v2 fields that use PARSE_INT (daytrade
    // counts, etc.) are well within int32 range.
    out = static_cast<int>(it->second.get<double>());
    return true;
}

inline bool obj_get_uint(const glz::generic::object_t& obj, std::string_view key, unsigned int& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    double v = it->second.get<double>();
    if (v < 0.0) {
        return false;
    }
    out = static_cast<unsigned int>(v);
    return true;
}

inline bool obj_get_uint64(const glz::generic::object_t& obj, std::string_view key, std::uint64_t& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    double v = it->second.get<double>();
    if (v < 0.0) {
        return false;
    }
    out = static_cast<std::uint64_t>(v);
    return true;
}

inline bool obj_get_double(const glz::generic::object_t& obj, std::string_view key, double& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    out = it->second.get<double>();
    return true;
}

inline bool obj_get_float(const glz::generic::object_t& obj, std::string_view key, float& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return false;
    }
    out = static_cast<float>(it->second.get<double>());
    return true;
}

inline bool obj_get_vector_doubles(const glz::generic::object_t& obj, std::string_view key, std::vector<double>& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    const glz::generic::array_t& arr = it->second.get_array();
    std::vector<double> items;
    items.reserve(arr.size());
    for (const glz::generic& elem : arr) {
        if (elem.is_number()) {
            items.push_back(elem.get<double>());
        }
    }
    out = std::move(items);
    return true;
}

inline bool obj_get_vector_uint64(const glz::generic::object_t& obj, std::string_view key,
                                  std::vector<std::uint64_t>& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    const glz::generic::array_t& arr = it->second.get_array();
    std::vector<std::uint64_t> items;
    items.reserve(arr.size());
    for (const glz::generic& elem : arr) {
        if (elem.is_number()) {
            double v = elem.get<double>();
            if (v >= 0.0) {
                items.push_back(static_cast<std::uint64_t>(v));
            }
        }
    }
    out = std::move(items);
    return true;
}

inline bool obj_get_vector_strings(const glz::generic::object_t& obj, std::string_view key,
                                   std::vector<std::string>& out) {
    glz::generic::object_t::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->second.is_array()) {
        return false;
    }
    const glz::generic::array_t& arr = it->second.get_array();
    std::vector<std::string> items;
    items.reserve(arr.size());
    for (const glz::generic& elem : arr) {
        if (elem.is_string()) {
            items.push_back(elem.get<std::string>());
        }
    }
    out = std::move(items);
    return true;
}

// Serialize an arbitrary glz::generic AST node back to a JSON string. Used by
// the model dispatchers (`Snapshot`, `Bars`-by-symbol, etc.) that hand a
// nested object's JSON text to a nested model's fromJSON(). Mirrors the
// RapidJSON `StringBuffer + Writer + node.Accept(writer)` pattern.
inline std::string write(const glz::generic& node) {
    std::string out;
    (void)glz::write_json(node, out);
    return out;
}

}  // namespace alpaca::markets::json_detail

// PARSE_* macros - replacements for the RapidJSON-era helpers in this same
// file. Each macro takes `node` (a `const glz::generic::object_t&` named
// exactly `node`) implicitly. The fromJSON() prelude that constructs `node`
// is provided by JSON_FROMJSON_PRELUDE below.

#define PARSE_STRING(var, name) ::alpaca::markets::json_detail::obj_get_string(node, (name), (var))

#define PARSE_INT(var, name) ::alpaca::markets::json_detail::obj_get_int(node, (name), (var))

#define PARSE_UINT(var, name) ::alpaca::markets::json_detail::obj_get_uint(node, (name), (var))

#define PARSE_UINT64(var, name) ::alpaca::markets::json_detail::obj_get_uint64(node, (name), (var))

#define PARSE_BOOL(var, name) ::alpaca::markets::json_detail::obj_get_bool(node, (name), (var))

#define PARSE_DOUBLE(var, name) ::alpaca::markets::json_detail::obj_get_double(node, (name), (var))

#define PARSE_FLOAT(var, name) ::alpaca::markets::json_detail::obj_get_float(node, (name), (var))

#define PARSE_VECTOR_DOUBLES(var, name) ::alpaca::markets::json_detail::obj_get_vector_doubles(node, (name), (var))

#define PARSE_VECTOR_UINT64(var, name) ::alpaca::markets::json_detail::obj_get_vector_uint64(node, (name), (var))

#define PARSE_VECTOR_STRINGS(var, name) ::alpaca::markets::json_detail::obj_get_vector_strings(node, (name), (var))

// Standard prelude for fromJSON() methods. Parses the body into a
// glz::generic root, returns a parse-error Status if invalid or non-object,
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
#define JSON_FROMJSON_PRELUDE(TAG)                                                    \
    ::glz::generic _glaze_root{};                                                     \
    {                                                                                 \
        ::glz::error_ctx _ec = ::glz::read_json(_glaze_root, json);                   \
        if (_ec) {                                                                    \
            return Status(1, "Received parse error when deserializing " TAG " JSON"); \
        }                                                                             \
    }                                                                                 \
    if (!_glaze_root.is_object()) {                                                   \
        return Status(1, "Deserialized valid JSON but it wasn't a " TAG " object");   \
    }                                                                                 \
    const ::glz::generic::object_t& node = _glaze_root.get_object()
