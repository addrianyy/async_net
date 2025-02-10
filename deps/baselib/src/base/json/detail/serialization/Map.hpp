#pragma once
#include "Core.hpp"
#include "Primitive.hpp"

#include <map>
#include <string>
#include <unordered_map>

namespace base::json {

namespace detail {

template <typename K, typename V, typename T>
static std::optional<Value> serialize_map_like_container(const T& container) {
  if constexpr (std::is_same_v<K, std::string>) {
    Object serialized;
    serialized.reserve(container.size());

    for (const auto& [k, v] : container) {
      auto serialized_value = json::serialize(v);
      if (!serialized_value) {
        return std::nullopt;
      }
      serialized.set(k, std::move(*serialized_value));
    }

    return serialized;
  } else {
    Array serialized;
    serialized.reserve(container.size());

    for (const auto& [k, v] : container) {
      auto serialized_key = json::serialize(k);
      if (!serialized_key) {
        return std::nullopt;
      }
      auto serialized_value = json::serialize(v);
      if (!serialized_value) {
        return std::nullopt;
      }

      Array entry{{
        std::move(*serialized_key),
        std::move(*serialized_value),
      }};

      serialized.append(std::move(entry));
    }

    return serialized;
  }
}

template <typename K, typename V, typename T, bool Reserve>
static std::optional<T> deserialize_map_like_container(const Value& value) {
  if constexpr (std::is_same_v<K, std::string>) {
    const auto object = value.object();
    if (!object) {
      return std::nullopt;
    }

    T deserialized;
    if constexpr (Reserve) {
      deserialized.reserve(object->size());
    }

    for (const auto& [k, v] : *object) {
      auto deserialized_value = json::deserialize<V>(v);
      if (!deserialized_value) {
        return std::nullopt;
      }

      if (!deserialized.emplace(std::string(k), std::move(*deserialized_value)).second) {
        return std::nullopt;
      }
    }

    return deserialized;
  } else {
    const auto array = value.array();
    if (!array) {
      return std::nullopt;
    }

    T deserialized;
    if constexpr (Reserve) {
      deserialized.reserve(array->size());
    }

    for (const auto& entry : *array) {
      const auto entry_array = entry.array();
      if (!entry_array || entry_array->size() != 2) {
        return std::nullopt;
      }

      auto deserialized_key = json::deserialize<K>((*entry_array)[0]);
      if (!deserialized_key) {
        return std::nullopt;
      }

      auto deserialized_value = json::deserialize<V>((*entry_array)[1]);
      if (!deserialized_value) {
        return std::nullopt;
      }

      if (!deserialized.emplace(std::move(*deserialized_key), std::move(*deserialized_value))
             .second) {
        return std::nullopt;
      }
    }

    return deserialized;
  }
}

}  // namespace detail

template <typename K, typename V>
struct Serializer<std::unordered_map<K, V>> {
  static std::optional<Value> serialize(const std::unordered_map<K, V>& value) {
    return detail::serialize_map_like_container<K, V>(value);
  }
};

template <typename K, typename V>
struct Deserializer<std::unordered_map<K, V>> {
  static std::optional<std::unordered_map<K, V>> deserialize(const Value& value) {
    return detail::deserialize_map_like_container<K, V, std::unordered_map<K, V>, true>(value);
  }
};

template <typename K, typename V>
struct Serializer<std::map<K, V>> {
  static std::optional<Value> serialize(const std::map<K, V>& value) {
    return detail::serialize_map_like_container<K, V>(value);
  }
};

template <typename K, typename V>
struct Deserializer<std::map<K, V>> {
  static std::optional<std::map<K, V>> deserialize(const Value& value) {
    return detail::deserialize_map_like_container<K, V, std::map<K, V>, false>(value);
  }
};

}  // namespace base::json
