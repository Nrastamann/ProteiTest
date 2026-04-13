#include "xlr.hpp"
#include <variant>
#include "mncs_messages.hpp"
#include "utility.hpp"

namespace mncs {

void XLR::hlr()
{
  while (!_invalid_state) {
    while (_tohlr.size() > 0) {
      auto* tohlr_msg = _tohlr.front();
      std::visit(
          utility::Visitor{
              [this](AuthInfoRequest& msg) {
                if (_data.contains(msg._imsi)) {
                  _data.at(msg._imsi)._tmsi = msg._imsi;
                  _fromhlr.push(AuthInfoResponse{._imsi = msg._imsi, ._tmsi = msg._tmsi});
                  return;
                }
                _data.insert({msg._imsi, XLRData{._tmsi = msg._tmsi}});
              },
              [this](LocationUpdateReq& msg) {
                auto& ref = _data.at(msg._imsi);
                ref._last_enodebid = msg._enodeb_id;
                ref._mmeid = msg._mme_id;
                ref._msisdn = msg._msisdn;
                _fromhlr.push(LocationUpdateAns{._tmsi = ref._tmsi});
              },
              [this](CancelLocationReq& msg) {
                for (auto& dataex : _data) {
                  if (msg._msisdn == dataex.second._msisdn) {
                    _fromhlr.push(CancelLocationResp{._tmsi = dataex.second._tmsi});
                  }
                }
              },
              [this](RouteRequest& msg) {}},
          *tohlr_msg);
    }
  }
}

}  // namespace mncs
