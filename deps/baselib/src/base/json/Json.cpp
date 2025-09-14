#include "Json.hpp"

#include <limits>

using namespace base::json;

Object::Object(Map entries)
    : entries_(std::move(entries)) {}

Value* Object::get(std::string_view key) {
  const auto it = entries_.find(key);
  return it != entries_.end() ? &it->second : nullptr;
}

const Value* Object::get(std::string_view key) const {
  const auto it = entries_.find(key);
  return it != entries_.end() ? &it->second : nullptr;
}

bool Object::contains(std::string_view key) const {
  return entries_.contains(key);
}

bool Object::set(std::string key, Value value) {
  const size_t size_before = entries_.size();
  entries_[std::move(key)] = std::move(value);
  return entries_.size() > size_before;
}

bool Object::remove(std::string_view key) {
  auto it = entries_.find(key);
  if (it != entries_.end()) {
    entries_.erase(it);
    return true;
  }
  return false;
}

void Object::reserve(size_t size) {
  entries_.reserve(size);
}

void Object::clear() {
  entries_.clear();
}

size_t Object::size() const {
  return entries_.size();
}

bool Object::empty() const {
  return entries_.empty();
}

Array::Array(Vector entries)
    : entries_(std::move(entries)) {}

void Array::append(Value value) {
  entries_.push_back(std::move(value));
}

size_t Array::size() const {
  return entries_.size();
}

bool Array::empty() const {
  return entries_.empty();
}

Value& Array::operator[](size_t index) {
  return entries_[index];
}

const Value& Array::operator[](size_t index) const {
  return entries_[index];
}

Array::operator std::span<Value>() {
  return std::span<Value>(entries_.data(), entries_.size());
}

Array::operator std::span<const Value>() const {
  return std::span<const Value>(entries_.data(), entries_.size());
}

void Array::remove(size_t index) {
  entries_.erase(entries_.begin() + ptrdiff_t(index));
}

void Array::pop() {
  entries_.pop_back();
}

void Array::resize(size_t new_size) {
  entries_.resize(new_size);
}

void Array::reserve(size_t size) {
  entries_.reserve(size);
}

void Array::clear() {
  entries_.clear();
}

Value& Array::front() {
  return entries_.front();
}

const Value& Array::front() const {
  return entries_.front();
}

Value& Array::back() {
  return entries_.back();
}

const Value& Array::back() const {
  return entries_.back();
}

void Number::set_int(int64_t value) {
  format_ = Format::Int;
  i_value = uint64_t(value);
  d_value = {};
}

void Number::set_uint(uint64_t value) {
  format_ = (value > uint64_t(std::numeric_limits<int64_t>::max())) ? Format::UInt : Format::Int;
  i_value = value;
  d_value = {};
}

void Number::set_double(double value) {
  format_ = Format::Double;
  i_value = {};
  d_value = value;
}

std::optional<int64_t> Number::get_int() const {
  if (format_ == Format::Int) {
    return int64_t(i_value);
  }
  return std::nullopt;
}

std::optional<uint64_t> Number::get_uint() const {
  if (format_ == Format::UInt) {
    return i_value;
  }
  if (format_ == Format::Int && int64_t(i_value) >= 0) {
    return i_value;
  }
  return std::nullopt;
}

std::optional<double> Number::get_double() const {
  if (format_ == Format::Double) {
    return d_value;
  }
  if (format_ == Format::Int) {
    return double(int64_t(i_value));
  }
  if (format_ == Format::UInt) {
    return double(i_value);
  }
  return std::nullopt;
}
