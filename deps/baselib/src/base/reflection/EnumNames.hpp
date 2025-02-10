#pragma once
#include <string_view>

namespace base::reflection {

template <typename T>
struct EnumToName {
  //  static std::optional<std::string_view> enum_to_name(const T& value) {
  //  }
};

template <typename T>
struct NameToEnum {
  //  static std::optional<T> name_to_enum(std::string_view name) {
  //  }
};

template <typename T>
concept HasEnumToName = requires(T t) { EnumToName<T>::enum_to_name(t); };

template <typename T>
concept HasNameToEnum = requires(T t) { NameToEnum<T>::name_to_enum(std::string_view{}); };

template <typename T>
concept HasEnumNames = requires(T t) {
  EnumToName<T>::enum_to_name(t);
  NameToEnum<T>::name_to_enum(std::string_view{});
};

}  // namespace base::reflection