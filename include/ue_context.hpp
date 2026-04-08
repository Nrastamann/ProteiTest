#pragma once

#include <cstdint>

namespace ue {
struct DeviceConfiguration {  //maybe inheritance?
  DeviceConfiguration(uint64_t imei, uint64_t msisdn, uint64_t imsi)
      : _imei(imei), _msisdn(msisdn), _imsi(imsi)
  {
  }
  [[nodiscard]] uint64_t imei() const { return _imei; }
  [[nodiscard]] uint64_t msisdn() const { return _msisdn; }
  [[nodiscard]] uint64_t imsi() const { return _imsi; }

 private:
  uint64_t _imei;
  uint64_t _msisdn;
  uint64_t _imsi;
};

class UeContext : public DeviceConfiguration {
 public:
  UeContext(uint32_t tmsi, uint64_t ttl_ue, uint64_t imei, uint64_t msisdn, uint64_t imsi)
      : DeviceConfiguration(imei, msisdn, imsi), _ttl_ue(ttl_ue), _tmsi(tmsi)
  {
  }
  UeContext(DeviceConfiguration& context, uint32_t tmsi, uint64_t ttl_ue)
      : DeviceConfiguration(context), _ttl_ue(ttl_ue), _tmsi(tmsi)
  {
  }
  [[nodiscard]] uint32_t tmsi() const { return _tmsi; }
  [[nodiscard]] uint64_t ttlUe() const { return _ttl_ue; }

 private:
  uint64_t _ttl_ue;
  uint32_t _tmsi;
};
};  // namespace ue
