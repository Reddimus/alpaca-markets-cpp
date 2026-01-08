#pragma once

#include <rapidjson/document.h>

#include <string>
#include <vector>

// JSON parsing macros for consistent deserialization
#define PARSE_STRING(var, name)          \
    if (d.HasMember(name) && d[name].IsString()) { \
        var = d[name].GetString();       \
    }

#define PARSE_INT(var, name)             \
    if (d.HasMember(name) && d[name].IsInt()) { \
        var = d[name].GetInt();          \
    }

#define PARSE_UINT(var, name)            \
    if (d.HasMember(name) && d[name].IsUint()) { \
        var = d[name].GetUint();         \
    }

#define PARSE_UINT64(var, name)          \
    if (d.HasMember(name) && d[name].IsUint64()) { \
        var = d[name].GetUint64();       \
    }

#define PARSE_BOOL(var, name)            \
    if (d.HasMember(name) && d[name].IsBool()) { \
        var = d[name].GetBool();         \
    }

#define PARSE_DOUBLE(var, name)          \
    if (d.HasMember(name) && d[name].IsNumber()) { \
        var = d[name].GetDouble();       \
    }

#define PARSE_FLOAT(var, name)           \
    if (d.HasMember(name) && d[name].IsNumber()) { \
        var = d[name].GetFloat();        \
    }

#define PARSE_VECTOR_DOUBLES(var, name)  \
    if (d.HasMember(name) && d[name].IsArray()) { \
        std::vector<double> items;       \
        for (auto& item : d[name].GetArray()) { \
            if (item.IsNumber()) {       \
                items.push_back(item.GetDouble()); \
            }                            \
        }                                \
        var = items;                     \
    }

#define PARSE_VECTOR_UINT64(var, name)   \
    if (d.HasMember(name) && d[name].IsArray()) { \
        std::vector<uint64_t> items;     \
        for (auto& item : d[name].GetArray()) { \
            if (item.IsNumber()) {       \
                items.push_back(item.GetUint64()); \
            }                            \
        }                                \
        var = items;                     \
    }

#define PARSE_VECTOR_STRINGS(var, name)  \
    if (d.HasMember(name) && d[name].IsArray()) { \
        std::vector<std::string> items;  \
        for (auto& item : d[name].GetArray()) { \
            if (item.IsString()) {       \
                items.push_back(item.GetString()); \
            }                            \
        }                                \
        var = items;                     \
    }
