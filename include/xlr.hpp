#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include "nlohmann/json.hpp"
namespace mncs {
struct XLRData {
  uint64_t _imei;
  uint64_t _msisdn;
  size_t _mmeid;
  size_t _last_enodebid;
  uint32_t _tmsi;
};
constexpr std::string_view kPathToXlr{"xlr/xlr.json"};

class XLR {
 public:
  ~XLR()
  {
    nlohmann::json jsn;
    for (auto& i : _data) {
      jsn += {{"MSISDN", i.second._msisdn},
              {"IMEI", i.second._imei},
              {"IMSI", i.first},
              {"MME_ID", i.second._mmeid},
              {"TMSI", i.second._tmsi},
              {"EnodeB-id", i.second._last_enodebid}};
    }
    std::string str = jsn.dump();
    std::ofstream ofstr(std::filesystem::path{kPathToXlr});
    if (ofstr.is_open()) {
      std::cout << "Couldn't save xlr\n";
      return;
    }
    ofstr << str;
    ofstr.close();
  }
  uint64_t authReq(uint64_t imsi, uint32_t tmsi) { _data[imsi]._tmsi = tmsi; }

  void auth(uint64_t imei, uint64_t msisdn, uint64_t imsi)
  {
    _data[imsi]._imei = imei;
    _data[imsi]._msisdn = msisdn;
  }

  bool updateLocation(uint64_t imsi, uint64_t msisdn, size_t mmeid, size_t enodebid)
  {
    auto it = _data.find(imsi);
    if (it != _data.end()) {
      if (it->second._msisdn != msisdn) {
        return false;
      }
      it->second._last_enodebid = enodebid;
      it->second._mmeid = mmeid;

      return true;
    }
    return false;
  }

  uint32_t getTmsi(uint64_t msisdn)
  {
    for (auto& i : _data) {
      if (i.second._msisdn == msisdn) {
        return i.second._tmsi;
      }
    }
  }

  XLR()
  {
    if (!std::filesystem::exists(kPathToXlr.substr(0, 4))) {
      if (!std::filesystem::create_directory(kPathToXlr.substr(0, 4))) {
        std::cout << "Couldn't create xlr directory\n";
        _invalid_state = true;
        return;
      }
    }
    std::ifstream ofs;
    if (!std::filesystem::exists(kPathToXlr)) {
      ofs.open(std::filesystem::path{kPathToXlr});
      ofs.close();
      return;
    }
    ofs.open(std::filesystem::path({kPathToXlr}));
    if (!ofs.is_open()) {
      std::cout << "Couldn't open xlr file\n";
      _invalid_state = true;
      return;
    }

    std::string str(std::istreambuf_iterator<char>{ofs}, {});
    nlohmann::json data = nlohmann::json::parse(str);

    for (auto& i : data.items()) {
      std::cout << i << ' ';
    }
    std::cout << '\n';
    ofs.close();
  }

 private:
  std::unordered_map<uint64_t, XLRData> _data;
  bool _invalid_state{false};
};
}  // namespace mncs
