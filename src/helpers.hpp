#pragma once
#include <algorithm>
#include <cassert>
namespace helpers {

/// @brief Return true if a vector-like `container` inclues `value`.
template <typename Container, typename Value>
inline auto contains(const Container& container, const Value& value) -> bool {
  return std::find(container.begin(), container.end(), value) != container.end();
}

/// @brief Remove a single item from a vector.  The value must exist in the container.
template <typename T>
inline auto remove(std::vector<T>& vec, const T& value_to_remove) {
  const auto it = std::find(vec.begin(), vec.end(), value_to_remove);
  assert(it != vec.end());
  vec.erase(it);
}
}  // namespace helpers
