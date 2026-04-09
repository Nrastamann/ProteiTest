#pragma once
#include "rigtorp/SPSCQueue.h"
namespace mncs {
class UEConnection {
 public:
  void pushToBase() {}
  void pushToUe() {}

 private:
  //ptr to enodeb list
  uint64_t _imei;
  uint64_t _imsi;
  uint32_t _tmsi;
  int _socket;
};
}  // namespace mncs
