#pragma once
#include "Core.hpp"

#include <utility>

namespace base::json {

template <typename T, typename U>
struct Serializer<std::pair<T, U>> {
  static std::optional<Value> serialize(const std::pair<T, U>& value) {
    auto serialized_1 = json::serialize(value.first);
    if (!serialized_1) {
      return std::nullopt;
    }

    auto serialized_2 = json::serialize(value.second);
    if (!serialized_2) {
      return std::nullopt;
    }

    return Array{{
      std::move(*serialized_1),
      std::move(*serialized_2),
    }};
  }
};

template <typename T, typename U>
struct Deserializer<std::pair<T, U>> {
  static std::optional<std::pair<T, U>> deserialize(const Value& value) {
    const auto array = value.array();
    if (!array || array->size() != 2) {
      return std::nullopt;
    }

    auto deserialized_1 = json::deserialize<T>((*array)[0]);
    if (!deserialized_1) {
      return std::nullopt;
    }

    auto deserialized_2 = json::deserialize<U>((*array)[1]);
    if (!deserialized_2) {
      return std::nullopt;
    }

    return std::pair<T, U>{std::move(*deserialized_1), std::move(*deserialized_2)};
  }
};

}  // namespace base::json
