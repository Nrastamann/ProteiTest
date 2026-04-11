#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <set>
#include <variant>
#include <vector>
#include "mncs_basestation.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"
#include "xlr.hpp"
namespace mncs {
class MME {
  static constexpr size_t kDefaultTtl{120000};
  static constexpr size_t kMMELength{15};
  static constexpr size_t kHandoverQueueLength{15};
  using transaction_id = uint64_t;  //imsi or tmsi
  using enodeb_index = uint64_t;
  using state_type = std::variant<uint64_t>;
  using procedure_state = std::pair<state_type, enodeb_index>;
  using msg_handover_type = uint64_t;
  using msg_type = uint64_t;

 public:
  void startMME();
  void pushToMME();
  void pushToHandover();
  void processMessages();

 private:
  alignas(utility::kCacheLength) std::atomic<uint64_t> _id_counter{0};
  //need to handle it propperly
  std::set<std::pair<uint64_t, utility::Spinlock>> _handover_locks;

  rigtorp::SPSCQueue<msg_type> _message_queue{kMMELength};
  rigtorp::SPSCQueue<msg_handover_type> _handover_queue{kHandoverQueueLength};
  std::unordered_map<transaction_id, procedure_state> _state_map;
  pr_utils::Timer _ttlepc{kDefaultTtl};
  uint64_t _ttl;
  uint64_t _mme_id;
  bool _is_on;
  //std::vector<int> sockets connections to other mme
};
}  // namespace mncs
