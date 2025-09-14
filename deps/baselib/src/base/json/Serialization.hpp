#pragma once
#include "detail/serialization/Core.hpp"
#include "detail/serialization/List.hpp"
#include "detail/serialization/Map.hpp"
#include "detail/serialization/Optional.hpp"
#include "detail/serialization/Pair.hpp"
#include "detail/serialization/Primitive.hpp"
#include "detail/serialization/SumType.hpp"

// template <>
// struct base::json::Serializer<Type> {
//   static std::optional<base::json::Value> serialize(const Type& value) { return std::nullopt; }
// };
//
// template <>
// struct base::json::Deserializer<Type> {
//   static std::optional<Type> deserialize(const base::json::Value& value) { return std::nullopt; }
// };
