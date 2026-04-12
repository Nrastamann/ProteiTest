#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <variant>
namespace messages::ue {
static const size_t kMaxSMSLenTemp{480};
//sms req/send to send across network

struct SmsReqNet {
  uint64_t _msisdn;
  std::array<char, kMaxSMSLenTemp> _sms;
  size_t _smsid;
  uint32_t _tmsi;
};

enum class MessageFlag : uint8_t {
  SMSSend,            //Send/receive text
  SMSStatus,          //send/receive Status
  MeasureRequest,     //Request enodeb power
  MeasureResponse,    //respone enodeb power
  AttachRequest,      //Attach req
  AttachResponse,     //Attach resp
  AuthRequest,        //Auth req
  AuthResponse,       //AuthResp
  MeasurementReport,  //reconnect to more powerfull enodeb
  ConfigurationResp,  //Configuration resp
};

enum class SMSStatus : uint8_t {
  Lost,
  Waiting,
  Received,
};

struct SmsUe {
  uint64_t _msisdn;
  std::string _sms;
};
//sms status
struct AcknowledgmentResponse {
  uint32_t _tmsi;
  size_t _message_id;
  SMSStatus _status;
};

//tmsi_d status to mme
struct AcknowledgmentRequest {
  uint32_t _tmsi_d;
  size_t _sms_id;
};

//measurement for enodeb power send
struct MeasurementRequest {
  uint64_t _imei;
  int64_t _x;
};

struct EnodeBInfo {
  uint64_t _enodeb_id;
  uint64_t _enodeb_power;
};

//result off measurementreq
//for future use
struct MeasurementResponse {
  uint64_t _imei;
  EnodeBInfo _info;
};

//picked enodeb connect_to
struct MeasurementReport {
  uint64_t _enodeb_idx;
};
enum class EnodeBStatus : uint8_t { CONNECT, DISCONNECT };
//config message
struct ConfigResponse {
  uint64_t _imei;
  uint64_t _ttl;
  EnodeBStatus _status;
};

//config message
struct AttachRequest {
  uint64_t _imei;
  uint64_t _imsi;
  uint64_t _msisdn;
};

//tmsi
struct AttachResponse {};

//config message
struct AuthRequest {
  uint32_t _tmsi;
};

//attached done, correctly
struct AuthResponse {
  uint64_t _imei;
  uint32_t _tmsi;
};

inline constexpr size_t kMsgDataSize{sizeof(SmsReqNet)};

using UEMessageData =
    std::variant<SmsReqNet, AcknowledgmentResponse, AcknowledgmentRequest, MeasurementRequest,
                 MeasurementResponse, MeasurementReport, AttachRequest, AttachResponse,
                 AuthRequest, AuthResponse, ConfigResponse, std::array<char, kMsgDataSize>>;

struct UEMessage {
  MessageFlag _msg_type;
  UEMessageData _data;
};

inline constexpr size_t kMsgSize{sizeof(UEMessage)};
}  // namespace messages::ue
