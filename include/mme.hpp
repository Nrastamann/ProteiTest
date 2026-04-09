#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include "timer.hpp"
namespace mncs {
class MME {
 public:
  uint32_t getTMSI(uint64_t imsi) { return static_cast<uint32_t>(std::hash<uint64_t>{}(imsi)); }
  void run();
  std::vector<pr_utils::Timer> _connectionTTL;
  std::vector<pr_utils::Timer> _smsTTL;
  pr_utils::Timer _ttlepc;

 private:
  uint64_t _ttl;
};
}  // namespace mncs
