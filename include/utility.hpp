#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
namespace utility {
template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};
inline constexpr size_t kCacheLength = {std::hardware_destructive_interference_size};
static constexpr size_t kMaxSMSLen{480};  //max message send len equals 512 byte
//sms req/send inside objects
struct SmsReq {
  uint64_t _tmsi;
  uint64_t _msisdn;
  uint64_t _sms_id;
  std::string _sms;
};

//sms req/send to send across network
struct SmsReqNet {
  uint64_t _tmsi;
  uint64_t _msisdn;
  std::array<char, kMaxSMSLen> _sms;
};

enum class MessageFlag : uint8_t {
  WRONG_RESPONSE_FLAG,
  SMSSend,        //Send/receive text
  SMSStatus,      //send/receive Status
  RangeReq,       //Request enodeb power
  RangeResp,      //respone enodeb power
  AttachReq,      //Attach req
  AttachResp,     //Attach resp
  AuthReq,        //Auth req
  AuthResp,       //AuthResp
  Reconnect,      //reconnect to more powerfull enodeb
  Configuration,  //Configuration resp
};
enum class SMSStatus : uint8_t {
  Lost,
  Received,
};

//sms status
struct AcknowledgmentReq {
  uint64_t _tmsi;
  SMSStatus _status;
};

//tmsi_d status to mme
struct AcknowledgmentUE {
  uint64_t _tmsi_d;
};

//measurement for enodeb power send
struct MeasurementReq {
  uint64_t _imei;
  uint64_t _x;
};

struct EnodeBInfo {
  uint64_t _enodeb_id;
  double _enodeb_power;
};

//result off measurementreq
//for future use
struct MeasurementControl {
  uint64_t _imei;
  EnodeBInfo _info;
};

//picked enodeb connect_to
struct MeasurementReport {
  uint64_t _enodeb_idx;
};
//config message
struct ConfigReq {
  uint64_t _imei;
  uint64_t _ttl;
};

//config message
struct AttachReq {
  uint64_t _imei;
  uint64_t _imsi;
  uint64_t _msisdn;
};

//tmsi
struct AttachResponse {
  uint64_t _tmsi;
};

//config message
struct AuthReq {
  uint64_t _imei;
  uint64_t _tmsi;
};
//attached done, correctly
struct AttachResult {};
using UEMessageData = std::variant<SmsReqNet, AcknowledgmentReq, AcknowledgmentUE,
                                   MeasurementReq, MeasurementControl, MeasurementReport,
                                   AttachReq, AttachResponse, AuthReq, AttachResult, ConfigReq>;

struct UEMessage {
  MessageFlag _msg_type;
  UEMessageData _data;
};
inline constexpr size_t kMsgSize{sizeof(UEMessage)};
};  // namespace utility
