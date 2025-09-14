#pragma once
#include <string_view>

namespace base::reflection {

template <typename T>
struct StructureFields {
  //  static void fields(auto&& field, auto& v) {
  //    field(...);
  //  }
};

template <typename T>
concept HasStructureFields =
  requires(T t) { StructureFields<T>::fields([](std::string_view name, auto&& member) {}, t); };

}  // namespace base::reflection
