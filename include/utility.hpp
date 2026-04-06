#pragma once

namespace utility {
template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};
};  // namespace utility
