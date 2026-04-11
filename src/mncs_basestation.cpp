#include "mncs_basestation.hpp"
#include <mutex>
#include "utility.hpp"
namespace mncs {

uint64_t BaseStation::getPower(int64_t pos) const
{
  return kMaxPower - (std::abs(_x - pos) / _radius);
}
size_t BaseStation::calculateMessageID(std::string_view str, size_t msisdn)
{
  return std::hash<size_t>{}(std::hash<std::string_view>{}(str) + std::hash<size_t>{}(msisdn));
}

}  // namespace mncs
