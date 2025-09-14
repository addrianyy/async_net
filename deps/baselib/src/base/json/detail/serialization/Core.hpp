#pragma once
#include <base/json/Json.hpp>
#include <base/reflection/EnumNames.hpp>
#include <base/reflection/StructureFields.hpp>

#include <optional>
#include <type_traits>

namespace base::json {

template <typename T>
class Serializer;

template <typename T>
class Deserializer;

template <typename T>
std::optional<Value> serialize(const T& value) {
  using Type = std::remove_cvref_t<T>;

  if constexpr (std::is_enum_v<Type>) {
    if constexpr (reflection::HasEnumNames<Type>) {
      const auto name = reflection::EnumToName<Type>::enum_to_name(value);
      if (name) {
        return json::serialize(*name);
      } else {
        return std::nullopt;
      }
    } else {
      return Serializer<std::underlying_type_t<Type>>::serialize(
        std::underlying_type_t<Type>(value)
      );
    }
  } else {
    return Serializer<Type>::serialize(value);
  }
}

template <typename T>
std::optional<T> deserialize(const Value& value) {
  using Type = std::remove_cvref_t<T>;

  if constexpr (std::is_enum_v<Type>) {
    if constexpr (reflection::HasEnumNames<Type>) {
      const auto name = json::deserialize<std::string>(value);
      if (name) {
        return reflection::NameToEnum<Type>::name_to_enum(*name);
      } else {
        return std::nullopt;
      }
    } else {
      auto deserialized = Deserializer<std::underlying_type_t<Type>>::deserialize(value);
      if (deserialized) {
        return Type(*deserialized);
      } else {
        return std::nullopt;
      }
    }
  } else {
    return Deserializer<Type>::deserialize(value);
  }
}

template <typename T>
struct Serializer {
  static std::optional<Value> serialize(const T& value) {
    static_assert(
      reflection::HasStructureFields<T>,
      "type doesn't have custom serializer or reflection::StructureFields defined"
    );

    Object serialized;
    bool success{true};

    reflection::StructureFields<T>::fields(
      [&](std::string_view name, const auto& field) {
        auto serialized_field = json::serialize(field);
        if (serialized_field) {
          if (!serialized.set(std::string(name), std::move(*serialized_field))) {
            success = false;
          }
        } else {
          success = false;
        }
      },
      value
    );

    if (success) {
      return serialized;
    } else {
      return std::nullopt;
    }
  }
};

template <typename T>
struct Deserializer {
  static std::optional<T> deserialize(const Value& value) {
    static_assert(
      reflection::HasStructureFields<T>,
      "type doesn't have custom deserializer or reflection::StructureFields defined"
    );

    const auto object = value.object();
    if (!object) {
      return std::nullopt;
    }

    T deserialized{};
    bool success{true};

    reflection::StructureFields<T>::fields(
      [&](std::string_view name, auto& field) {
        auto serialized_field = object->get(name);
        if (serialized_field) {
          auto deserialized_field =
            json::deserialize<std::remove_cvref_t<decltype(field)>>(*serialized_field);
          if (deserialized_field) {
            field = std::move(*deserialized_field);
          } else {
            success = false;
          }
        } else {
          success = false;
        }
      },
      deserialized
    );

    if (success) {
      return deserialized;
    } else {
      return std::nullopt;
    }
  }
};

}  // namespace base::json
