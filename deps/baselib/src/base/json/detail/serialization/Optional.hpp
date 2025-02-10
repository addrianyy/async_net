#pragma once
#include "Core.hpp"

#include <optional>

namespace base::json {

template <typename T>
struct Serializer<std::optional<T>> {
  static std::optional<Value> serialize(const std::optional<T>& value) {
    if (value) {
      return json::serialize(*value);
    } else {
      return json::Null{};
    }
  }
};

template <typename T>
struct Deserializer<std::optional<T>> {
  static std::optional<std::optional<T>> deserialize(const Value& value) {
    if (value.is_null()) {
      return std::optional<std::optional<T>>{std::optional<T>{}};
    } else {
      auto deserialized = json::deserialize<T>(value);
      if (deserialized) {
        return std::optional<std::optional<T>>{std::optional<T>{std::move(*deserialized)}};
      } else {
        return std::nullopt;
      }
    }
  }
};

}  // namespace base::json
