#pragma once
#include "Core.hpp"

#include <base/containers/SumType.hpp>

namespace base::json {

namespace detail {

template <typename T, typename CurrentVariant, typename... Rest>
static std::optional<T> deserialize_variant(
  typename T::VariantIdType variant_id, const Value& value
) {
  if (base::VariantIdFor<CurrentVariant>::id == variant_id) {
    auto deserialized = json::deserialize<CurrentVariant>(value);
    if (deserialized) {
      return T{std::move(*deserialized)};
    } else {
      return std::nullopt;
    }
  } else {
    if constexpr (sizeof...(Rest) > 0) {
      return deserialize_variant<T, Rest...>(variant_id, value);
    } else {
      return std::nullopt;
    }
  }
}

}  // namespace detail

template <typename... Args>
struct Serializer<base::SumType<Args...>> {
  static std::optional<Value> serialize(const base::SumType<Args...>& value) {
    auto serialized_type = json::serialize(value.id());
    if (!serialized_type) {
      return std::nullopt;
    }

    auto serialized_value = value.visit([&](const auto& value) -> std::optional<Value> {
      auto serialized_value = json::serialize(value);
      if (!serialized_value) {
        return std::nullopt;
      }

      return std::optional{*serialized_value};
    });
    if (!serialized_value) {
      return std::nullopt;
    }

    Object object;
    object.reserve(2);
    object.set("type", std::move(*serialized_type));
    object.set("value", std::move(*serialized_value));

    return object;
  }
};

template <typename... Args>
struct Deserializer<base::SumType<Args...>> {
  static std::optional<base::SumType<Args...>> deserialize(const Value& value) {
    const auto object = value.object();
    if (!object) {
      return std::nullopt;
    }

    const auto serialized_type = object->get("type");
    if (!serialized_type) {
      return std::nullopt;
    }

    const auto serialized_value = object->get("value");
    if (!serialized_value) {
      return std::nullopt;
    }

    using VariantId = typename base::SumType<Args...>::VariantIdType;

    const auto variant_id = json::deserialize<VariantId>(*serialized_type);
    if (!variant_id) {
      return std::nullopt;
    }

    return detail::deserialize_variant<base::SumType<Args...>, Args...>(
      *variant_id, *serialized_value
    );
  }
};

}  // namespace base::json
