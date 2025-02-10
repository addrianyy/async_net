#pragma once
#include "Core.hpp"

#include <string>
#include <string_view>

namespace base::json {

#define NUMBER_SERIALIZATION(type)                               \
  template <>                                                    \
  struct Serializer<type> {                                      \
    static std::optional<Value> serialize(type value) {          \
      return json::Number{value};                                \
    }                                                            \
  };                                                             \
                                                                 \
  template <>                                                    \
  struct Deserializer<type> {                                    \
    static std::optional<type> deserialize(const Value& value) { \
      return value.get<type>();                                  \
    }                                                            \
  };

NUMBER_SERIALIZATION(uint8_t)
NUMBER_SERIALIZATION(uint16_t)
NUMBER_SERIALIZATION(uint32_t)
NUMBER_SERIALIZATION(uint64_t)

NUMBER_SERIALIZATION(int8_t)
NUMBER_SERIALIZATION(int16_t)
NUMBER_SERIALIZATION(int32_t)
NUMBER_SERIALIZATION(int64_t)

NUMBER_SERIALIZATION(float)
NUMBER_SERIALIZATION(double)

#undef NUMBER_SERIALIZATION

template <>
struct Serializer<bool> {
  static std::optional<Value> serialize(bool value) { return json::Boolean{value}; }
};

template <>
struct Deserializer<bool> {
  static std::optional<bool> deserialize(const Value& value) { return value.get<bool>(); }
};

template <>
struct Serializer<char> {
  static std::optional<Value> serialize(char value) { return json::Number{uint8_t(value)}; }
};

template <>
struct Deserializer<char> {
  static std::optional<char> deserialize(const Value& value) {
    auto number = value.get<uint8_t>();
    if (number) {
      return char(*number);
    } else {
      return std::nullopt;
    }
  };
};

template <>
struct Serializer<std::string> {
  static std::optional<Value> serialize(const std::string& value) { return json::String{value}; }
};

template <>
struct Deserializer<std::string> {
  static std::optional<std::string> deserialize(const Value& value) {
    auto string = value.get<std::string_view>();
    if (string) {
      return std::string(*string);
    } else {
      return std::nullopt;
    }
  }
};

template <>
struct Serializer<std::string_view> {
  static std::optional<Value> serialize(std::string_view value) {
    return json::String{std::string(value)};
  }
};

}  // namespace base::json
