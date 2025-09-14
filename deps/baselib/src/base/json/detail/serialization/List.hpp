#pragma once
#include "Core.hpp"
#include "Primitive.hpp"

#include <array>
#include <set>
#include <span>
#include <unordered_set>
#include <vector>

namespace base::json {

namespace detail {

template <typename T>
static std::optional<Value> serialize_list_like_container(const T& container) {
  std::vector<Value> elements;
  elements.reserve(container.size());

  for (const auto& e : container) {
    auto serialized = json::serialize(e);
    if (!serialized) {
      return std::nullopt;
    }
    elements.push_back(std::move(*serialized));
  }

  return json::Array{std::move(elements)};
}

template <typename E, typename SizeCallback, typename ElementCallback>
static bool deserialize_list_like_container(
  const Value& value, SizeCallback&& size_callback, ElementCallback&& element_callback
) {
  const auto array = value.array();
  if (!array) {
    return false;
  }

  if (!size_callback(array->size())) {
    return false;
  }

  for (const auto& e : *array) {
    auto deserialized = json::deserialize<E>(e);
    if (!deserialized) {
      return false;
    }
    element_callback(std::move(*deserialized));
  }

  return true;
}

}  // namespace detail

template <typename E>
struct Serializer<std::span<const E>> {
  static std::optional<Value> serialize(const std::span<const E>& value) {
    return detail::serialize_list_like_container(value);
  }
};

template <typename E>
struct Serializer<std::vector<E>> {
  static std::optional<Value> serialize(const std::vector<E>& value) {
    return detail::serialize_list_like_container(value);
  }
};

template <typename E>
struct Deserializer<std::vector<E>> {
  static std::optional<std::vector<E>> deserialize(const Value& value) {
    std::vector<E> elements;
    const auto result = detail::deserialize_list_like_container<E>(
      value,
      [&](size_t size) {
        elements.reserve(size);
        return true;
      },
      [&](E element) { elements.push_back(std::move(element)); }
    );

    if (result) {
      return elements;
    } else {
      return std::nullopt;
    }
  }
};

template <typename E, size_t N>
struct Serializer<std::array<E, N>> {
  static std::optional<Value> serialize(const std::array<E, N>& value) {
    return detail::serialize_list_like_container(value);
  }
};

template <typename E, size_t N>
struct Deserializer<std::array<E, N>> {
  static std::optional<std::array<E, N>> deserialize(const Value& value) {
    std::array<E, N> elements{};
    size_t i{};
    const auto result = detail::deserialize_list_like_container<E>(
      value,
      [&](size_t size) { return elements.size() == size; },
      [&](E element) { elements[i++] = std::move(element); }
    );

    if (result) {
      return elements;
    } else {
      return std::nullopt;
    }
  }
};

template <typename E>
struct Serializer<std::set<E>> {
  static std::optional<Value> serialize(const std::set<E>& value) {
    return detail::serialize_list_like_container(value);
  }
};

template <typename E>
struct Deserializer<std::set<E>> {
  static std::optional<std::set<E>> deserialize(const Value& value) {
    std::set<E> elements;
    const auto result = detail::deserialize_list_like_container<E>(
      value,
      [&](size_t size) { return true; },
      [&](E element) { elements.insert(std::move(element)); }
    );

    if (result) {
      return elements;
    } else {
      return std::nullopt;
    }
  }
};

template <typename E>
struct Serializer<std::unordered_set<E>> {
  static std::optional<Value> serialize(const std::unordered_set<E>& value) {
    return detail::serialize_list_like_container(value);
  }
};

template <typename E>
struct Deserializer<std::unordered_set<E>> {
  static std::optional<std::unordered_set<E>> deserialize(const Value& value) {
    std::unordered_set<E> elements;
    const auto result = detail::deserialize_list_like_container<E>(
      value,
      [&](size_t size) {
        elements.reserve(size);
        return true;
      },
      [&](E element) { elements.insert(std::move(element)); }
    );

    if (result) {
      return elements;
    } else {
      return std::nullopt;
    }
  }
};

}  // namespace base::json
