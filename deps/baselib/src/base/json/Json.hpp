#pragma once
#include <base/containers/SumType.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace base::json {

namespace detail {

struct string_hash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;

  std::size_t operator()(const char* str) const { return hash_type{}(str); }
  std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
  std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

}  // namespace detail

enum class ValueType {
  Object,
  Array,
  String,
  Number,
  Boolean,
  Null,
};

class Value;

class Object {
 public:
  using Map = std::unordered_map<std::string, Value, detail::string_hash, std::equal_to<>>;

 private:
  Map entries_;

 public:
  constexpr static ValueType variant_id = ValueType::Object;

  Object() = default;
  explicit Object(Map entries);

  Value* get(std::string_view key);
  const Value* get(std::string_view key) const;

  template <typename T>
  T* get(std::string_view key);
  template <typename T>
  const T* get(std::string_view key) const;

  template <typename T>
  std::optional<T> get_number(std::string_view key) const;

  bool contains(std::string_view key) const;

  bool set(std::string key, Value value);
  bool remove(std::string_view key);

  void reserve(size_t size);

  void clear();

  size_t size() const;
  bool empty() const;

  auto begin() { return entries_.begin(); }
  auto end() { return entries_.end(); }

  auto begin() const { return entries_.begin(); }
  auto end() const { return entries_.end(); }

  Map& entries() { return entries_; }
  const Map& entries() const { return entries_; }
};

class Array {
 public:
  using Vector = std::vector<Value>;

 private:
  Vector entries_;

 public:
  constexpr static ValueType variant_id = ValueType::Array;

  Array() = default;
  explicit Array(Vector entries);

  void append(Value value);
  void remove(size_t index);
  void pop();

  void resize(size_t new_size);
  void reserve(size_t size);

  void clear();

  size_t size() const;
  bool empty() const;

  Value& front();
  const Value& front() const;

  Value& back();
  const Value& back() const;

  Value& operator[](size_t index);
  const Value& operator[](size_t index) const;

  auto begin() { return entries_.begin(); }
  auto end() { return entries_.end(); }

  auto begin() const { return entries_.begin(); }
  auto end() const { return entries_.end(); }

  auto rbegin() { return entries_.rbegin(); }
  auto rend() { return entries_.rend(); }

  auto rbegin() const { return entries_.rbegin(); }
  auto rend() const { return entries_.rend(); }

  Vector& entries() { return entries_; }
  const Vector& entries() const { return entries_; }

  operator std::span<Value>();
  operator std::span<const Value>() const;
};

class String {
  std::string string_;

 public:
  constexpr static ValueType variant_id = ValueType::String;

  String() = default;
  String(std::string string)
      : string_(std::move(string)) {}

  std::string& get() { return string_; }
  const std::string& get() const { return string_; }

  void set(std::string value) { string_ = std::move(value); }

  operator std::string_view() const { return string_; }
};

class Number {
 public:
  enum class Format {
    Int,
    UInt,
    Double,
  };

 private:
  Format format_{Format::Int};
  uint64_t i_value{};
  double d_value{};

 public:
  constexpr static ValueType variant_id = ValueType::Number;

  Number() = default;

  template <typename T>
  Number(T value) {
    set(value);
  }

  Format format() const { return format_; }

  template <typename T>
  void set(T value) {
    constexpr auto is_floating_point = std::is_same_v<T, double> || std::is_same_v<T, float>;
    constexpr auto is_integer = std::is_integral_v<T>;

    static_assert(is_floating_point || is_integer, "T must be integer or floating point");

    if constexpr (is_floating_point) {
      set_double(double(value));
    } else if constexpr (is_integer) {
      constexpr auto is_signed = std::is_signed_v<T>;
      constexpr auto is_unsigned = std::is_unsigned_v<T>;

      static_assert(is_signed || is_unsigned, "T is invalid integer");

      if constexpr (is_signed) {
        set_int(int64_t(value));
      } else if constexpr (is_unsigned) {
        set_uint(uint64_t(value));
      }
    }
  }

  template <typename T>
  std::optional<T> get() const {
    constexpr auto is_floating_point = std::is_same_v<T, double> || std::is_same_v<T, float>;
    constexpr auto is_integer = std::is_integral_v<T>;

    static_assert(is_floating_point || is_integer, "T must be integer or floating point");

    if constexpr (is_integer) {
      constexpr auto is_signed = std::is_signed_v<T>;
      constexpr auto is_unsigned = std::is_unsigned_v<T>;

      static_assert(is_signed || is_unsigned, "T is invalid integer");

      if constexpr (is_signed) {
        const auto value = get_int();
        if (!value) {
          return std::nullopt;
        }

        if (*value < int64_t(std::numeric_limits<T>::min()) ||
            *value > int64_t(std::numeric_limits<T>::max())) {
          return std::nullopt;
        }

        return T(*value);
      } else if constexpr (is_unsigned) {
        const auto value = get_uint();
        if (!value) {
          return std::nullopt;
        }

        if (*value > uint64_t(std::numeric_limits<T>::max())) {
          return std::nullopt;
        }

        return T(*value);
      }
    } else if constexpr (is_floating_point) {
      const auto value = get_double();
      if (!value) {
        return std::nullopt;
      }
      return T(*value);
    }

    return std::nullopt;
  }

  void set_int(int64_t value);
  void set_uint(uint64_t value);
  void set_double(double value);

  std::optional<int64_t> get_int() const;
  std::optional<uint64_t> get_uint() const;
  std::optional<double> get_double() const;
};

class Boolean {
  bool value_ = false;

 public:
  constexpr static ValueType variant_id = ValueType::Boolean;

  Boolean() = default;
  Boolean(bool value)
      : value_(value) {}

  bool get() const { return value_; }
  void set(bool value) { value_ = value; }

  operator bool() const { return value_; }
};

class Null {
 public:
  constexpr static ValueType variant_id = ValueType::Null;

  Null() = default;
};

class Value : public base::SumType<Object, Array, String, Number, Boolean, Null> {
 public:
  using SumType::SumType;

  Value()
      : SumType(Null{}) {}

  ValueType type() const { return id(); }

  bool is_object() const { return is<Object>(); }
  bool is_array() const { return is<Array>(); }
  bool is_string() const { return is<String>(); }
  bool is_number() const { return is<Number>(); }
  bool is_boolean() const { return is<Boolean>(); }
  bool is_null() const { return is<Null>(); }

  Object* object() { return as<Object>(); }
  Array* array() { return as<Array>(); }
  String* string() { return as<String>(); }
  Number* number() { return as<Number>(); }
  Boolean* boolean() { return as<Boolean>(); }
  Null* null() { return as<Null>(); }

  const Object* object() const { return as<Object>(); }
  const Array* array() const { return as<Array>(); }
  const String* string() const { return as<String>(); }
  const Number* number() const { return as<Number>(); }
  const Boolean* boolean() const { return as<Boolean>(); }
  const Null* null() const { return as<Null>(); }

  template <typename T>
  std::optional<T> get() const {
    constexpr auto is_native_number = std::is_integral_v<T> || std::is_floating_point_v<T>;
    constexpr auto is_native_bool = std::is_same_v<T, bool>;
    constexpr auto is_native_string = std::is_same_v<T, std::string_view>;

    static_assert(
      is_native_number || is_native_bool || is_native_string,
      "T must be one of: numeric type, boolean or std::string_view"
    );

    if constexpr (is_native_number && !is_native_bool) {
      const auto value = number();
      return value ? value->get<T>() : std::nullopt;
    } else if constexpr (is_native_bool) {
      const auto value = boolean();
      return value ? std::optional{value->get()} : std::nullopt;
    } else if constexpr (is_native_string) {
      const auto value = string();
      return value ? std::optional{std::string_view{value->get()}} : std::nullopt;
    }
  }
};

template <typename T>
T* Object::get(std::string_view key) {
  auto value = get(key);
  return value ? value->as<T>() : nullptr;
}

template <typename T>
const T* Object::get(std::string_view key) const {
  auto value = get(key);
  return value ? value->as<T>() : nullptr;
}

template <typename T>
std::optional<T> Object::get_number(std::string_view key) const {
  auto value = get<Number>(key);
  return value ? value->get<T>() : std::nullopt;
}

}  // namespace base::json
