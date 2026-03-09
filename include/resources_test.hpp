#pragma once

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <cassert>
#include <filesystem>
#include <format>
#include <string_view>
#include "ip_addr.hpp"
#include "logger.hpp"

class ITest {
 public:
  virtual bool operator()() = 0;

  ITest() = default;
  ITest(const ITest&) = default;
  ITest(ITest&&) = default;
  ITest& operator=(const ITest&) = default;
  ITest& operator=(ITest&&) = default;
  virtual ~ITest() = default;
};

class ResourceTest final : ITest {
 public:
  explicit ResourceTest(std::span<std::string_view> resources)
      : _resources(resources)
  {
  }

  ~ResourceTest() final = default;
  ResourceTest(const ResourceTest&) = default;
  ResourceTest(ResourceTest&&) = default;
  ResourceTest& operator=(const ResourceTest&) = default;
  ResourceTest& operator=(ResourceTest&&) = default;

  bool operator()() override
  {
    bool value = true;
    if (_resources.size() != 0 &&
        !std::ranges::all_of(
            _resources.begin(), _resources.end(),
            [](std::string_view sv) { return std::filesystem::exists(sv); })) {

      logger_presets::acquiringResourceError<ResourceTest>(
          std::format("{}", _resources));
      value = false;
    }

    return value;
  };

 private:
  std::span<std::string_view> _resources;
};

class ConnectionTest final : ITest {
 public:
  ~ConnectionTest() final = default;
  ConnectionTest(const ConnectionTest&) = default;
  ConnectionTest(ConnectionTest&&) = default;
  ConnectionTest& operator=(const ConnectionTest&) = default;
  ConnectionTest& operator=(ConnectionTest&&) = default;
  explicit ConnectionTest(std::span<IpAddr> addresses)
  {
    for (auto& addr : addresses) {
      _resources.push_back(addr);
    }
  }

  bool operator()() override
  {
    bool value = true;

    auto test_resource = [](const IpAddr& addr) {
      using namespace boost::asio;

      boost::asio::io_context service;
      ip::tcp::endpoint ep(ip::make_address_v4(addr._addr),
                           static_cast<unsigned short>(addr._port));
      ip::tcp::socket sock(service);
      boost::system::error_code err;
      err = sock.connect(ep, err);

      if (err) {
        return false;
      }

      sock.close();
      return true;
    };

    if (_invalid_state || (_resources.size() != 0 &&
                           !std::ranges::all_of(_resources, test_resource))) {
      std::string output_str;
      for (auto& ip_addr : _resources) {
        output_str += std::format("{}\n", ip_addr);
      }
      logger_presets::acquiringResourceError<ConnectionTest>(output_str);

      value = false;
    }

    return value;
  };

 private:
  std::vector<IpAddr> _resources;

  bool _invalid_state = false;
};
