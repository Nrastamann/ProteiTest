#pragma once

#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include "rigtorp/SPSCQueue.h"
#include "utility.hpp"

namespace data_storage {
class DataPool {
  using value_type = std::string;
  using return_type = std::string;
  using return_reference_type = return_type&;

  using const_return_reference_type = const return_type&;
  using container_type = std::unordered_map<
      size_t, std::tuple<std::string, utility::SMSStatus, uint64_t>>;  //tmsi_src+hash

  using container_type_ref = container_type&;

  size_t getHash(std::string_view str, size_t msisdn) const
  {
    return std::hash<size_t>{}(std::hash<std::string_view>{}(str) +
                               std::hash<size_t>{}(_counter + msisdn));
  }

 public:
  [[nodiscard]] size_t size() const { return _storage.size(); }

  auto find(size_t hash) { return _storage.find(hash); }
  auto end() { return _storage.end(); }
  auto begin() { return _storage.begin(); }

  void pushSms(utility::SmsReqNet&& str)
  {
    if (_message_queue.size() == _send_sms_queue.capacity()) {
      flush();
    }
    _message_queue.push(std::move(str));
  }

  void pushStatus(utility::AcknowledgmentResp&& str)
  {
    if (_status_queue.size() == _send_sms_queue.capacity()) {
      flush();
    }

    _status_queue.push(std::move(str));
  }

  void pushNewSms(utility::SmsUe&& str)
  {
    if (_send_sms_queue.size() == _send_sms_queue.capacity()) {
      flush();
    }

    _send_sms_queue.push(std::move(str));
  }

  void flush()
  {
    std::lock_guard<std::mutex> lock(_flush_mtx);
    while (_message_queue.size() != 0) {
      auto* str = _message_queue.front();
      _storage.insert({str->_smsid, std::tuple{std::string(str->_sms.data()),
                                               utility::SMSStatus::Received, str->_msisdn}});
      _message_queue.pop();
    }

    while (_send_sms_queue.size() != 0) {
      auto* str = _send_sms_queue.front();
      size_t hash = getHash(str->_sms, str->_msisdn);
      _counter++;
      _storage.insert({hash, std::tuple{str->_sms, utility::SMSStatus::Waiting, str->_msisdn}});

      _send_sms_queue.pop();
    }

    while (_status_queue.size() != 0) {
      auto* str = _status_queue.front();
      std::get<1>(_storage[str->_message_id]) = str->_status;
      _message_queue.pop();
    }
  }

  container_type_ref getContainer() { return _storage; }
  [[nodiscard]] container_type getContainer() const { return _storage; }
  void printAll(uint64_t msisdn)
  {
    std::cout << '\n';
    std::lock_guard<std::mutex> lock(_flush_mtx);
    for (auto& msg : _storage) {
      uint64_t msisdn_msg = std::get<2>(msg.second);
      std::cout << "Message ";
      std::cout << (msisdn == msisdn_msg ? "from " : "to ") << msisdn_msg << ": "
                << std::get<0>(msg.second) << "  | ";

      std::string_view status;

      switch (std::get<1>(msg.second)) {
        case utility::SMSStatus::Waiting:
          status = "Still waiting.\n";
          break;
        case utility::SMSStatus::Lost:
          status = "Lost.\n";
          break;
        case utility::SMSStatus::Received:
          status = "Received.\n";
          break;
      }
      std::cout << status;
    }
    std::cout << '\n';
  }

 private:
  static constexpr size_t kQueueCapacity{16};

  alignas(utility::kCacheLength) std::mutex _flush_mtx;
  container_type _storage;

  rigtorp::SPSCQueue<utility::SmsUe> _send_sms_queue{kQueueCapacity};
  rigtorp::SPSCQueue<utility::AcknowledgmentResp> _status_queue{kQueueCapacity};
  rigtorp::SPSCQueue<utility::SmsReqNet> _message_queue{kQueueCapacity};
  size_t _counter{0};
};
}  // namespace data_storage
