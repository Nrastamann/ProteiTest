#pragma once
template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};
